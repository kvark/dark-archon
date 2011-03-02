namespace bwt

import System
import OpenTK.Graphics.OpenGL


public class Archon:
	# shader handles and uniform locations
	private	final	kInit	as Kernel
	private final	kSort	as Kernel
	private final	kSum	as Kernel
	private final	kDiff	as Kernel
	private final	kEven	as Kernel
	private final	kOff	as Kernel
	private final	kFill	as Kernel
	private final	kOut	as Kernel
	private	final	locSortTex		as int
	private	final	locDiffTex		as int
	private	final	locFillScale	as int
	private	final	locOutTex		as int
	# buffer objects
	private final	vao			as int
	private final	bufData		as int
	private final	bufIndex	as int
	private final	bufValue	as int
	private final	bufOut		as int
	private final	bufDebug	as int
	# others
	private final	tf		= TransFeedback(1)
	private final	texVal	= GL.GenTexture()
	private final	texTmp	= GL.GenTexture()
	private final	fbo		as int
	private final	qSamples	= Query( QueryTarget.SamplesPassed )
	# variables
	private	N		as int	= 0
	private off		as int	= 0
	private final	debug	= false
	
	# methods
	public def constructor():
		# init shaders
		kInit	= Kernel( ('sh/k_init.glv',), ('at_c0','at_c1','at_c2','at_c3'), ('to_index','to_value') )
		kSort	= Kernel( ('sh/k_sort.glv','sh/s_bubble.glv','sh/s_gate.glv'), ('at_ia','at_ib','at_ic'), ('to_index','to_debug') )
		locSortTex		= kSort.getUniform('unit_val')
		kSum	= Kernel( ('sh/k_sum.glv',), ('at_diff',), ('to_sum',) )
		kDiff	= Kernel( ('sh/k_diff.glv',), ('at_i0','at_i1'), ('to_diff',) )
		locDiffTex		= kDiff.getUniform('unit_val')
		kEven	= Kernel( ('sh/k_even.glv',), ('at_off',), ('to_one',) )
		kOff	= Kernel( ('sh/k_off.glv',), ('at_sum','at_one'), ('to_off',) )
		kFill	= Kernel( ('sh/k_fill.glv','sh/k_fill.glf'), ('at_val','at_ind'), null )
		locFillScale	= kFill.getUniform('scale')
		kOut	= Kernel( ('sh/k_out.glv',), ('at_ind',), ('to_sym',) )
		locOutTex		= kOut.getUniform('unit_data')
		# init buffers
		bufs = (-1,-1,-1,-1,-1)
		GL.GenVertexArrays(1,bufs)
		vao = bufs[0]
		GL.GenBuffers(bufs.Length,bufs)
		bufData		= bufs[0]
		bufIndex	= bufs[1]
		bufValue	= bufs[2]
		bufOut		= bufs[3]
		bufDebug	= bufs[4]
		GL.GenFramebuffers(1,bufs)
		fbo = bufs[0]
	
	def destructor():
		try:
			for t in (texVal,texTmp):
				GL.DeleteTexture(t)
			GL.DeleteBuffers(5,(bufData,bufIndex,bufValue,bufOut,bufDebug))
			x = (fbo,vao)
			GL.DeleteFramebuffers(1,x[0])
			GL.DeleteVertexArrays(1,x[1])
		except e as OpenTK.Graphics.GraphicsContextMissingException:
			pass
	
	# perform complete BWT, override input and return original index
	public def process(input as (byte)) as int:
		N = input.Length
		GL.BindVertexArray(vao)
		stage_load(input)	# input->Data
		cleanup()
		stage_init()		# Data->Value (first 4 symbols), init Index
		cleanup()
		jump = 4
		while true:
			stage_sort_bubble()	# Value,Index->Out->Index (sort)
			cleanup()
			break	if N>4 and jump>=N
			stage_diff()	# Value,Index->Out (diff)
			cleanup()
			stage_sum()		# Out->Value (sum bitree)
			cleanup()
			stage_off()		# Value->Out (off bitree)
			cleanup()
			stage_fill()	# Out,Index->Value (scatter back)
			cleanup()
			break	if N<=4
			jump *= 2
		stage_out()	# Value,Index->Out (BWT)
		cleanup()	# read back data and zero index
		return stage_exit(input)	# Out->output
	
	# implementation details
	
	private def readBuffer(data as (int), vbo as uint) as void:
		GL.BindBuffer( BufferTarget.ArrayBuffer, vbo )
		buf = GL.MapBuffer( BufferTarget.ArrayBuffer, BufferAccess.ReadOnly )
		Marshal.Copy( buf, data, 0, data.Length )
		GL.UnmapBuffer( BufferTarget.ArrayBuffer )
	
	private def cleanup() as void:
		for i in range(4):
			GL.DisableVertexAttribArray(i)
			GL.BindBufferBase( BufferTarget.TransformFeedbackBuffer, i, 0 )
		GL.BindBuffer( BufferTarget.ArrayBuffer, 0 )
		GL.BindBuffer( BufferTarget.ElementArrayBuffer, 0 )
		GL.BindFramebuffer( FramebufferTarget.Framebuffer, 0 )
	
	private def stage_load(input as (byte)) as void:
		GL.BindBuffer( BufferTarget.ArrayBuffer, bufData )
		GL.BufferData( BufferTarget.ArrayBuffer, IntPtr(N+4), IntPtr.Zero, BufferUsageHint.StaticDraw )
		# offset by 1 in order to allow getting the previous symbol
		GL.BufferSubData( BufferTarget.ArrayBuffer, IntPtr(1), IntPtr(N), input )
		assert N>=3
		GL.CopyBufferSubData( BufferTarget.ArrayBuffer, BufferTarget.ArrayBuffer, IntPtr(N), IntPtr(0), IntPtr(1) )
		GL.CopyBufferSubData( BufferTarget.ArrayBuffer, BufferTarget.ArrayBuffer, IntPtr(1), IntPtr(N+1), IntPtr(3) )
		for buf in (bufIndex,bufValue,bufOut,bufDebug):
			GL.BindBuffer( BufferTarget.ArrayBuffer, buf )
			GL.BufferData( BufferTarget.ArrayBuffer, IntPtr(4*N+4), IntPtr.Zero, BufferUsageHint.StaticDraw )
		# textures
		GL.BindTexture( TextureTarget.Texture2D, texTmp )
		GL.TexImage2D( TextureTarget.Texture2D, 0, PixelInternalFormat.R32i, N,1,0, PixelFormat.RedInteger, PixelType.Int, IntPtr.Zero )
		GL.BindTexture( TextureTarget.TextureBuffer, texVal )
		GL.TexBuffer( TextureBufferTarget.TextureBuffer, SizedInternalFormat.R32ui, bufValue )
		GL.BindFramebuffer( FramebufferTarget.Framebuffer, fbo )
		GL.FramebufferTexture( FramebufferTarget.Framebuffer, FramebufferAttachment.ColorAttachment0, texTmp, 0)
		status = GL.CheckFramebufferStatus( FramebufferTarget.Framebuffer )
		assert status == FramebufferErrorCode.FramebufferComplete

	private def stage_init() as void:
		GL.BindBuffer( BufferTarget.ArrayBuffer, bufData )
		for i in range(4): # bypassing uint stride=1 hardware limitation
			GL.EnableVertexAttribArray(i)
			GL.VertexAttribIPointer( i, 1, VertexAttribIPointerType.UnsignedByte, 1, IntPtr(i+1) )
		kInit.bind()
		GL.ActiveTexture( TextureUnit.Texture0 )
		GL.BindTexture( TextureTarget.TextureBuffer, texVal )
		GL.BindBufferBase( BufferTarget.TransformFeedbackBuffer, 0, bufIndex )
		GL.BindBufferBase( BufferTarget.TransformFeedbackBuffer, 1, bufValue )
		tf.draw(0,N)

	private def stage_sort_bubble() as void:
		GL.BindBuffer( BufferTarget.ArrayBuffer, bufValue )
		GL.CopyBufferSubData( BufferTarget.ArrayBuffer, BufferTarget.ArrayBuffer, IntPtr.Zero, IntPtr(N*4), IntPtr(4) )
		#bind index [-1,0,1]
		GL.BindBuffer( BufferTarget.ArrayBuffer, bufIndex )
		for i in range(3):
			GL.EnableVertexAttribArray(i)
			GL.VertexAttribIPointer( i, 1, VertexAttribIPointerType.Int, 0, IntPtr((i-1)*4) )
		kSort.bind()
		GL.Uniform1(locSortTex,0)
		for i in range(N+N-3):
			#readBuffer(dI,v_index)
			off = i & 1
			num = Math.Min(i,N+N-4-i) + 2-off
			ioff = IntPtr(off*4)
			inum = IntPtr(num*4)
			GL.BindBuffer( BufferTarget.ArrayBuffer, 0 )
			GL.BindBufferBase( BufferTarget.TransformFeedbackBuffer, 0, bufOut )
			GL.BindBufferBase( BufferTarget.TransformFeedbackBuffer, 1, bufDebug )
			tf.draw(off,num)
			GL.BindBuffer( BufferTarget.ArrayBuffer, bufOut )
			GL.BindBuffer( BufferTarget.ElementArrayBuffer, bufIndex )
			GL.CopyBufferSubData( BufferTarget.ArrayBuffer, BufferTarget.ElementArrayBuffer, IntPtr.Zero, ioff, inum )
		
	private def sort(number as int, step as int, getInd as callable(int) as int) as void:
		pass

	private def stage_sort_oem() as void:
		listLog = 0
		while N>>++listLog:
			step = 1<<(listLog-1)
			sort( N>>1, step ) do(i as int):
				chunk = i>>(listLog-1)
				rem = i&(step-1)
				return (chunk<<listLog) + rem
			while (step>>=1) >= 1:
				continue	if listLog==1
				numChunks = N>>listLog
				numIxPerChunk = ((1<<listLog) - (step<<1))>>1
				sort( numChunks*numIxPerChunk, step ) do(i as int):
					chunk = i / numIxPerChunk
					rem = i - chunk*numIxPerChunk
					swapChunk = rem / step
					swapRem = rem - swapChunk * step
					return (chunk<<listLog) + (2*swapChunk+1)*step + swapRem


	private def stage_diff() as void:
		GL.BindBuffer( BufferTarget.ArrayBuffer, bufIndex )
		for i in range(2):
			GL.EnableVertexAttribArray(i)
			GL.VertexAttribIPointer( i, 1, VertexAttribIPointerType.Int, 0, IntPtr(i*4) )
		kDiff.bind()
		GL.Uniform1(locDiffTex,0)
		GL.BindBufferRange( BufferTarget.TransformFeedbackBuffer, 0, bufOut, IntPtr(4), IntPtr(N*4) )
		tf.draw(0,N)
		GL.BindBuffer( BufferTarget.ArrayBuffer, bufOut )
		GL.CopyBufferSubData( BufferTarget.ArrayBuffer, BufferTarget.ArrayBuffer, IntPtr(N*4), IntPtr.Zero, IntPtr(4) )
		if debug:
			dV = array[of int](N+1)
			readBuffer(dV,bufOut)
			dV[0] = 0
	
	private def stage_sum() as void:
		assert not N&(N-1)	# power of 2
		GL.BindBuffer( BufferTarget.ArrayBuffer, bufOut )
		GL.EnableVertexAttribArray(0)
		GL.VertexAttribIPointer(0, 2, VertexAttribIPointerType.Int, 0, IntPtr.Zero )
		GL.BindBufferBase( BufferTarget.TransformFeedbackBuffer, 0, bufValue )
		kSum.bind()
		off = 0
		size = N
		while (size >>= 1):
			GL.BindBufferRange( BufferTarget.TransformFeedbackBuffer, 0, bufValue, IntPtr(off*4), IntPtr(size*4) )
			tf.draw(0,size)
			GL.BindBuffer( BufferTarget.ArrayBuffer, bufValue )
			GL.VertexAttribIPointer(0, 2, VertexAttribIPointerType.Int, 0, IntPtr(off*4) )
			off += size
			if debug:
				dV = array[of int](N)
				readBuffer(dV,bufValue)
			GL.BindBuffer( BufferTarget.ArrayBuffer, 0 )
		size = 0	# read v_value[off-1] = sum of differences
	
	private def stage_off() as void:
		size = o2 = 1
		GL.EnableVertexAttribArray(0)
		GL.EnableVertexAttribArray(1)
		if debug:
			dI = array[of int](N)
			dV = array[of int](N)
		else: dI=dV=null
		while o2>=0:
			off -= size
			o2 = off - size*2	# read offset
			kEven.bind()
			if o2<0:
				GL.BindBuffer( BufferTarget.ArrayBuffer, bufOut )
				GL.VertexAttribIPointer(0, 2, VertexAttribIPointerType.Int, 0, IntPtr.Zero )
			else:
				GL.BindBuffer( BufferTarget.ArrayBuffer, bufValue )
				GL.VertexAttribIPointer(0, 2, VertexAttribIPointerType.Int, 0, IntPtr(o2*4) )
			GL.BindBuffer( BufferTarget.ArrayBuffer, 0 )
			GL.BindBufferRange( BufferTarget.TransformFeedbackBuffer, 0, bufValue, IntPtr((off+size)*4), IntPtr(size*4) )
			tf.draw(0,size)
			if debug:
				readBuffer(dV,bufValue)
			kOff.bind()
			GL.BindBuffer( BufferTarget.ArrayBuffer, bufValue )
			GL.VertexAttribIPointer(0, 1, VertexAttribIPointerType.Int, 0, IntPtr(off*4) )
			GL.VertexAttribIPointer(1, 1, VertexAttribIPointerType.Int, 0, IntPtr((off+size)*4) )
			GL.BindBuffer( BufferTarget.ArrayBuffer, 0 )
			if o2<0:
				GL.BindBufferBase( BufferTarget.TransformFeedbackBuffer, 0, bufOut )
			else:
				GL.BindBufferRange( BufferTarget.TransformFeedbackBuffer, 0, bufValue, IntPtr(o2*4), IntPtr(size*2*4) )
			using tf.catch():
				GL.DrawArrays( BeginMode.Points, 0, size )
			if debug:
				if o2<0:	readBuffer(dI,bufOut)
				else:		readBuffer(dV,bufValue)
			size <<= 1
		assert not off
	
	private def stage_fill() as void:
		result = -1	# ClearBuffer doesn't work properly with non-zero values
		GL.BindFramebuffer( FramebufferTarget.Framebuffer, fbo )
		GL.DrawBuffer( DrawBufferMode.ColorAttachment0 )
		GL.Viewport(0,0,N,1)
		GL.ClearBuffer( ClearBuffer.Color, 0, result )
		kFill.bind()
		GL.Uniform1(locFillScale, 1f / N)
		using qSamples.catch():
			GL.BindBuffer( BufferTarget.ArrayBuffer, bufOut )
			GL.EnableVertexAttribArray(0)
			GL.VertexAttribIPointer( 0, 1, VertexAttribIPointerType.Int, 0, IntPtr.Zero )
			GL.BindBuffer( BufferTarget.ArrayBuffer, bufIndex )
			GL.EnableVertexAttribArray(1)
			GL.VertexAttribIPointer( 1, 1, VertexAttribIPointerType.Int, 0, IntPtr.Zero )
			GL.DrawArrays( BeginMode.Points, 0, N )
		result = qSamples.result()
		assert result == N
		GL.BindBuffer( BufferTarget.PixelPackBuffer, bufValue )
		GL.ReadPixels( 0,0,N,1, PixelFormat.RedInteger, PixelType.Int, IntPtr.Zero )
		GL.BindBuffer( BufferTarget.PixelPackBuffer, 0 )
		if debug:
			dV = array[of int](N)
			readBuffer(dV,bufValue)
			dV[0]=0

	private def stage_out() as void:
		GL.BindTexture( TextureTarget.TextureBuffer, texVal )
		GL.TexBuffer( TextureBufferTarget.TextureBuffer, SizedInternalFormat.R8ui, bufData )
		GL.EnableVertexAttribArray(0)
		GL.BindBuffer( BufferTarget.ArrayBuffer, bufIndex )
		# working in groups of 4 because TF doesn't allow single byte attrib output
		GL.VertexAttribIPointer( 0, 4, VertexAttribIPointerType.Int, 0, IntPtr.Zero )
		kOut.bind()
		GL.Uniform1(locOutTex,0)
		GL.BindBufferBase( BufferTarget.TransformFeedbackBuffer, 0, bufOut )
		assert not (N&3)
		tf.draw(0,N>>2)

	private def stage_exit(data as (byte)) as int:
		assert data.Length == N
		zero = (of int:-1)
		readBuffer(zero,bufValue)
		GL.BindBuffer( BufferTarget.ArrayBuffer, bufOut )
		buf = GL.MapBuffer( BufferTarget.ArrayBuffer, BufferAccess.ReadOnly )
		Marshal.Copy( buf, data, 0, N )
		GL.UnmapBuffer( BufferTarget.ArrayBuffer )
		GL.BindBuffer( BufferTarget.ArrayBuffer, 0 )
		return zero[0]
