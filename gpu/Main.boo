namespace bwt

import System
import OpenTK
import OpenTK.Graphics
import OpenTK.Graphics.OpenGL


def makeWindow() as GameWindow:
	dd = DisplayDevice.Default
	gm = GraphicsMode( ColorFormat(8), 0, 0 )
	conFlags  = GraphicsContextFlags.ForwardCompatible | GraphicsContextFlags.Debug
	gameFlags  = GameWindowFlags.Default
	return GameWindow(1,1,gm,'ArchonGPU',gameFlags,dd,3,3,conFlags)


def makeShader(names as (string), attribs as (string), feedback as (string)) as uint:
	status = -1
	log = ''
	prog = GL.CreateProgram()
	for str in names:
		type as ShaderType
		type = ShaderType.VertexShader		if str.EndsWith('.glv')
		type = ShaderType.GeometryShader	if str.EndsWith('.glg')
		type = ShaderType.FragmentShader	if str.EndsWith('.glf')
		sh = GL.CreateShader(type)
		text = IO.File.ReadAllText(str)
		GL.ShaderSource(sh,text)
		GL.CompileShader(sh)
		GL.GetShaderInfoLog(sh,log)
		GL.GetShader(sh, ShaderParameter.CompileStatus, status)
		assert status
		GL.AttachShader(prog,sh)
		GL.DeleteShader(sh)
	for i in range(attribs.Length):
		GL.BindAttribLocation(prog,i,attribs[i])
	if feedback:
		GL.TransformFeedbackVaryings( prog, feedback.Length, feedback, TransformFeedbackMode.SeparateAttribs )
	GL.LinkProgram(prog)
	GL.GetProgramInfoLog(prog,log)
	GL.GetProgram(prog, ProgramParameter.LinkStatus, status)
	assert status
	return prog


def readBuffer(data as (int), vbo as uint) as void:
	GL.BindBuffer( BufferTarget.ArrayBuffer, vbo )
	buf = GL.MapBuffer( BufferTarget.ArrayBuffer, BufferAccess.ReadOnly )
	Marshal.Copy( buf, data, 0, data.Length )
	GL.UnmapBuffer( BufferTarget.ArrayBuffer )


using win = makeWindow():
	print win.Title
	data = (of byte: 5,3,2,4)
	N = data.Length
	v_data = v_ar = -1
	GL.GenBuffers(1,v_data)
	GL.GenVertexArrays(1,v_ar)
	#0. load data
	GL.BindVertexArray(v_ar)
	GL.BindBuffer( BufferTarget.ArrayBuffer, v_data )
	GL.BufferData( BufferTarget.ArrayBuffer, IntPtr(N+3), IntPtr.Zero, BufferUsageHint.StaticDraw )
	GL.BufferSubData( BufferTarget.ArrayBuffer, IntPtr.Zero, IntPtr(N), data )
	GL.CopyBufferSubData( BufferTarget.ArrayBuffer, BufferTarget.ArrayBuffer, IntPtr.Zero, IntPtr(N), IntPtr(3) )
	#1a. prepare buffers
	v_index = v_value = -1
	GL.GenBuffers(1,v_index)
	GL.BindBuffer( BufferTarget.ArrayBuffer, v_index )
	GL.BufferData( BufferTarget.ArrayBuffer, IntPtr(4*N), IntPtr.Zero, BufferUsageHint.StaticDraw )
	GL.GenBuffers(1,v_value)
	GL.BindBuffer( BufferTarget.ArrayBuffer, v_value )
	GL.BufferData( BufferTarget.ArrayBuffer, IntPtr(4*N+4), IntPtr.Zero, BufferUsageHint.StaticDraw )
	#1b. fill I and V with TF
	sh_init = makeShader( ('sh/init.glv',), ('at_c0','at_c1','at_c2','at_c3'), ('to_index','to_value') )
	GL.BindBuffer( BufferTarget.ArrayBuffer, v_data )
	for i in range(4): # bypassing uint stride=1 hardware limitation
		GL.EnableVertexAttribArray(i)
		GL.VertexAttribIPointer( i, 1, VertexAttribIPointerType.UnsignedByte, 1, IntPtr(i) )
	GL.UseProgram(sh_init)
	GL.Enable( EnableCap.RasterizerDiscard )
	tf_query = -1
	GL.ActiveTexture( TextureUnit.Texture0 )
	tex_buf = GL.GenTexture()
	GL.BindTexture( TextureTarget.TextureBuffer, tex_buf )
	GL.TexBuffer( TextureBufferTarget.TextureBuffer, SizedInternalFormat.R32ui, v_value )
	GL.GenQueries(1,tf_query)
	GL.BindBufferBase( BufferTarget.TransformFeedbackBuffer, 0, v_index )
	GL.BindBufferBase( BufferTarget.TransformFeedbackBuffer, 1, v_value )
	GL.BeginTransformFeedback( BeginFeedbackMode.Points )
	GL.BeginQuery( QueryTarget.TransformFeedbackPrimitivesWritten, tf_query )
	GL.DrawArrays( BeginMode.Points, 0, data.Length )
	GL.EndQuery( QueryTarget.TransformFeedbackPrimitivesWritten )
	GL.EndTransformFeedback()
	for i in range(4):
		GL.DisableVertexAttribArray(i)
	for i in range(2):
		GL.BindBufferBase( BufferTarget.TransformFeedbackBuffer, i, 0 )
	#2. iterate
	sh_sort = makeShader( ('sh/sort.glv',), ('at_ia','at_ib','at_ic'), ('to_index',) )
	loc_tex = GL.GetUniformLocation(sh_sort,'unit_val')
	v_out = -1
	GL.GenBuffers(1,v_out)
	GL.BindBuffer( BufferTarget.ArrayBuffer, v_out )
	GL.BufferData( BufferTarget.ArrayBuffer, IntPtr(4*N), IntPtr.Zero, BufferUsageHint.StaticDraw )
	jump = 4
	while true:
		# 2a. sort by 2 keys
		GL.BindBuffer( BufferTarget.ArrayBuffer, v_value )
		GL.CopyBufferSubData( BufferTarget.ArrayBuffer, BufferTarget.ArrayBuffer, IntPtr.Zero, IntPtr(N*4), IntPtr(4) )
		//bind index [-1,0,1]
		GL.BindBuffer( BufferTarget.ArrayBuffer, v_index )
		for i in range(3):
			GL.EnableVertexAttribArray(i)
			GL.VertexAttribIPointer( i, 1, VertexAttribIPointerType.Int, 0, IntPtr((i-1)*4) )
		GL.UseProgram(sh_sort)
		GL.Uniform1(loc_tex,0)
		for i in range(N+N-3):
			dI = array[of int](N)
			readBuffer(dI,v_index)
			off = i & 1
			num = Math.Min(i,N+N-4-i) + 2-off
			ioff = IntPtr(off*4)
			inum = IntPtr(num*4)
			GL.BindBufferBase( BufferTarget.TransformFeedbackBuffer, 0, v_out )
			GL.BeginTransformFeedback( BeginFeedbackMode.Points )
			GL.BeginQuery( QueryTarget.TransformFeedbackPrimitivesWritten, tf_query )
			GL.DrawArrays( BeginMode.Points, off, num )
			GL.EndQuery( QueryTarget.TransformFeedbackPrimitivesWritten )
			GL.EndTransformFeedback()
			GL.BindBuffer( BufferTarget.ElementArrayBuffer, v_out )
			GL.CopyBufferSubData( BufferTarget.ElementArrayBuffer, BufferTarget.ArrayBuffer, IntPtr.Zero, ioff, inum )
			GL.BindBuffer( BufferTarget.ArrayBuffer, 0 )
		GL.BindBufferBase( BufferTarget.TransformFeedbackBuffer, 0, 0 )
		for i in range(3):
			GL.DisableVertexAttribArray(i)
		# 2b. update V
		# 2c. loop
		jump *= 2
		break	if jump>=N
	# check
	dI = array[of int](N)
	readBuffer(dI,v_index)
	dV = array[of int](N)
	readBuffer(dV,v_value)
	dI[0] = dV[0] = 0
	#3. read data back
