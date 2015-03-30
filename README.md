## Contents
This project aggregates all my BWT-oriented efforts. The released/abandoned stuff can be found in the downloads section, while the on-going developments are in the code repository. The list includes Archon-4 (a4), Archon-5 (a5), Archon-6 (a6), and Archon-GPU at the moment. It will be extended with the compression schemes eventually.

## Highlights
Archon is a general code name for my BWT algorithms deriving from the Itoh-Tanaka [work](http://web.iiit.ac.in/~abhishek_shukla/suffix/In%20Memory%20Suffix%20Array%20Construction.pdf). Some of them showed impressive results in the suffix sorting benchmark by Yuta Mori. Each new Archon version brings specific major improvements and is considered to be a branch.

- *a4* introduced new "defense" improvements to deal with long repetitions in the input. It was the first one to pass the [Gauntlet Corpus](http://www.michael-maniscalco.com/testset/gauntlet/) and the last one tested by Yuta.
- *a5* tries to go deeper past the 2nd order Itoh-Tanaka model. It explores IT-N variations, which allow lesser number of suffixes to be sorted directly.
- *a6* deals with redundant alphabets by compressing the input stream using fixed-length and Huffman codes.
- *a7* attempts to squeeze the juice out of the new [SA-IS](http://www.cs.sysu.edu.cn/nong/index.files/Two%20Efficient%20Algorithms%20for%20Linear%20Suffix%20Array%20Construction.pdf) method.

Aside from the traditional single-thread CPU algorithms you can find an OpenGL-based implementation of the doubling sort on GPU. It is very experimental and does not achieve the highest results at the moment but has some potential.

## Principles
- memory consumption should be `5N + O(1)`.
- performance should be consistent, e.g. the upper time bound should be as low as possible. Optimally: `O(N)`
- compression ratio should be on par with existing BWT compression schemes (YBS, SBC, DC, bzip, etc) and use as little data-specific code as possible.
