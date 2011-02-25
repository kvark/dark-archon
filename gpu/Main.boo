namespace bwt

import OpenTK
import OpenTK.Graphics


dd = DisplayDevice.Default
gm = GraphicsMode( ColorFormat(8), 0, 0 )
conFlags  = GraphicsContextFlags.ForwardCompatible | GraphicsContextFlags.Debug
gameFlags  = GameWindowFlags.Default

using win = GameWindow(1,1,gm,'ArchonGPU',gameFlags,dd,3,3,conFlags):
	print win.Title
	data = (of byte: 5,3,2,4)
	#data = System.IO.File.ReadAllBytes('input.bin')
	ar = Archon()
	result = ar.process(data)
	System.IO.File.WriteAllBytes('output.bin',data)
	result = 0
