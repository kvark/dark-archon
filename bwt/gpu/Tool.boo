namespace bwt

import System
import OpenTK.Graphics.OpenGL


#-----------#
#	KERNEL	#
#-----------#

public class Kernel:
	public final sid	as int
	public final log	as string
	
	public def constructor(names as (string), attribs as (string), feedback as (string)):
		status = -1
		sid = GL.CreateProgram()
		tmpLog = ''
		for str in names:
			type as ShaderType
			type = ShaderType.VertexShader		if str.EndsWith('.glv')
			type = ShaderType.GeometryShader	if str.EndsWith('.glg')
			type = ShaderType.FragmentShader	if str.EndsWith('.glf')
			sh = GL.CreateShader(type)
			text = IO.File.ReadAllText(str)
			GL.ShaderSource(sh,text)
			GL.CompileShader(sh)
			GL.GetShaderInfoLog(sh,tmpLog)
			GL.GetShader(sh, ShaderParameter.CompileStatus, status)
			assert status
			GL.AttachShader(sid,sh)
			GL.DeleteShader(sh)
		for i in range(attribs.Length):
			GL.BindAttribLocation(sid,i,attribs[i])
		if feedback:
			GL.TransformFeedbackVaryings( sid, feedback.Length, feedback, TransformFeedbackMode.SeparateAttribs )
		GL.LinkProgram(sid)
		GL.GetProgramInfoLog(sid,tmpLog)
		GL.GetProgram(sid, ProgramParameter.LinkStatus, status)
		log = tmpLog
		assert status
	
	public def getUniform(name as string) as int:
		return GL.GetUniformLocation(sid,name)
	public def bind() as void:
		GL.UseProgram(sid)


#-----------#
#	BUFFER	#
#-----------#

public class Buffer:
	pass


#-----------#
#	QUERY	#
#-----------#

public class Catcher(IDisposable):
	public final t	as QueryTarget
	public def constructor(q as Query):
		t = q.target
		GL.BeginQuery( t, q.qid )
	public virtual def Dispose() as void:
		GL.EndQuery(t)

public class Query:
	public final qid	as int
	public final target	as QueryTarget
	public def constructor( targ as QueryTarget ):
		tmp = 0
		GL.GenQueries(1,tmp)
		qid,target = tmp,targ
	def destructor():
		tmp = qid
		try: GL.DeleteQueries(1,tmp)
		except e as  OpenTK.Graphics.GraphicsContextMissingException:
			pass
	public virtual def catch() as Catcher:
		return Catcher(self)
	public def result() as int:
		rez = 0
		GL.GetQueryObject(qid, GetQueryObjectParam.QueryResult, rez)
		return rez


#-----------------------#
#	TRANSFORM FEEDBACK	#
#-----------------------#

public class CatcherFeed(Catcher):
	public def constructor(q as Query, m as BeginFeedbackMode):
		GL.Enable( EnableCap.RasterizerDiscard )
		GL.BeginTransformFeedback(m)
		super(q)
	public override def Dispose() as void:
		super()
		GL.EndTransformFeedback()
		GL.Disable( EnableCap.RasterizerDiscard )

public class TransFeedback(Query):
	public counter		= 0
	public final mode	as BeginFeedbackMode
	public def constructor(nv as byte):
		super( QueryTarget.TransformFeedbackPrimitivesWritten )
		mode = (BeginFeedbackMode.Points, BeginFeedbackMode.Lines, BeginFeedbackMode.Triangles)[nv-1]
	public override def catch() as Catcher:
		return CatcherFeed(self,mode)
	public def draw(off as int, num as int) as void:
		++counter
		using catch():
			GL.DrawArrays( BeginMode.Points, off, num )
		assert result() == num
