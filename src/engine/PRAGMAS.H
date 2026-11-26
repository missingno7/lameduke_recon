// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
// This file has been modified from Ken Silverman's original release

#pragma aux scale =\
	"imul ebx",\
	"idiv ecx",\
	parm [eax][ebx][ecx]\
	modify [eax edx]\

#pragma aux mulscale =\
	"imul ebx",\
	"shrd eax, edx, cl",\
	parm [eax][ebx][ecx]\
	modify [edx]\

#pragma aux mulscale1 =\
	"imul ebx",\
	"shrd eax, edx, 1",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale2 =\
	"imul ebx",\
	"shrd eax, edx, 2",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale3 =\
	"imul ebx",\
	"shrd eax, edx, 3",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale4 =\
	"imul ebx",\
	"shrd eax, edx, 4",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale5 =\
	"imul ebx",\
	"shrd eax, edx, 5",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale6 =\
	"imul ebx",\
	"shrd eax, edx, 6",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale7 =\
	"imul ebx",\
	"shrd eax, edx, 7",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale8 =\
	"imul ebx",\
	"shrd eax, edx, 8",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale9 =\
	"imul ebx",\
	"shrd eax, edx, 9",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale10 =\
	"imul ebx",\
	"shrd eax, edx, 10",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale11 =\
	"imul ebx",\
	"shrd eax, edx, 11",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale12 =\
	"imul ebx",\
	"shrd eax, edx, 12",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale13 =\
	"imul ebx",\
	"shrd eax, edx, 13",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale14 =\
	"imul ebx",\
	"shrd eax, edx, 14",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale15 =\
	"imul ebx",\
	"shrd eax, edx, 15",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale16 =\
	"imul ebx",\
	"shrd eax, edx, 16",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale17 =\
	"imul ebx",\
	"shrd eax, edx, 17",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale18 =\
	"imul ebx",\
	"shrd eax, edx, 18",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale19 =\
	"imul ebx",\
	"shrd eax, edx, 19",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale20 =\
	"imul ebx",\
	"shrd eax, edx, 20",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale21 =\
	"imul ebx",\
	"shrd eax, edx, 21",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale22 =\
	"imul ebx",\
	"shrd eax, edx, 22",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale23 =\
	"imul ebx",\
	"shrd eax, edx, 23",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale24 =\
	"imul ebx",\
	"shrd eax, edx, 24",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale25 =\
	"imul ebx",\
	"shrd eax, edx, 25",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale26 =\
	"imul ebx",\
	"shrd eax, edx, 26",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale27 =\
	"imul ebx",\
	"shrd eax, edx, 27",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale28 =\
	"imul ebx",\
	"shrd eax, edx, 28",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale29 =\
	"imul ebx",\
	"shrd eax, edx, 29",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale30 =\
	"imul ebx",\
	"shrd eax, edx, 30",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale31 =\
	"imul ebx",\
	"shrd eax, edx, 31",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux mulscale32 =\
	"imul ebx",\
	"mov eax, edx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux doublemulscale =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, cl",\
	parm [eax][ebx][esi][edi][ecx]\
	modify [edx]\

#pragma aux doublemulscale1 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 1",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale2 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 2",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale3 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 3",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale4 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 4",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale5 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 5",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale6 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 6",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale7 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 7",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale8 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 8",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale9 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 9",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale10 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 10",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale11 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 11",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale12 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 12",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale13 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 13",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale14 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 14",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale15 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 15",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale16 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 16",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale17 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 17",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale18 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 18",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale19 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 19",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale20 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 20",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale21 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 21",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale22 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 22",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale23 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 23",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale24 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 24",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale25 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 25",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale26 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 26",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale27 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 27",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale28 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 28",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale29 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 29",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale30 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 30",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale31 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"shrd eax, edx, 31",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux doublemulscale32 =\
	"imul ebx",\
	"xchg esi, eax",\
	"mov ebx, edx",\
	"imul edi",\
	"add eax, esi",\
	"adc edx, ebx",\
	"mov eax, edx",\
	parm [eax][ebx][esi][edi]\
	modify [edx]\

#pragma aux optimul =\
	"imul ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux boundmulscale =\
	"imul ebx",\
	"mov ebx, edx",\
	"shrd eax, edx, cl",\
	"sar edx, cl",\
	"xor edx, eax",\
	"js checkit",\
	"xor edx, eax",\
	"jz skipboundit",\
	"cmp edx, 0xffffffff",\
	"je skipboundit",\
	"checkit:",\
	"mov eax, ebx",\
	"sar eax, 31",\
	"add eax, 0x80000000",\
	"not eax",\
	"skipboundit:",\
	parm [eax][ebx][ecx]\
	modify [edx]\

#pragma aux divscale =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, cl",\
	"sal eax, cl",\
	"idiv ebx",\
	parm [eax][ebx][ecx]\
	modify [edx]\

#pragma aux divscale1 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 1",\
	"sal eax, 1",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale2 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 2",\
	"sal eax, 2",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale3 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 3",\
	"sal eax, 3",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale4 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 4",\
	"sal eax, 4",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale5 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 5",\
	"sal eax, 5",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale6 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 6",\
	"sal eax, 6",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale7 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 7",\
	"sal eax, 7",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale8 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 8",\
	"sal eax, 8",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale9 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 9",\
	"sal eax, 9",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale10 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 10",\
	"sal eax, 10",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale11 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 11",\
	"sal eax, 11",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale12 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 12",\
	"sal eax, 12",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale13 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 13",\
	"sal eax, 13",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale14 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 14",\
	"sal eax, 14",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale15 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 15",\
	"sal eax, 15",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale16 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 16",\
	"sal eax, 16",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale17 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 17",\
	"sal eax, 17",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale18 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 18",\
	"sal eax, 18",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale19 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 19",\
	"sal eax, 19",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale20 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 20",\
	"sal eax, 20",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale21 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 21",\
	"sal eax, 21",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale22 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 22",\
	"sal eax, 22",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale23 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 23",\
	"sal eax, 23",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale24 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 24",\
	"sal eax, 24",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale25 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 25",\
	"sal eax, 25",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale26 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 26",\
	"sal eax, 26",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale27 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 27",\
	"sal eax, 27",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale28 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 28",\
	"sal eax, 28",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale29 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 29",\
	"sal eax, 29",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale30 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 30",\
	"sal eax, 30",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale31 =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, 31",\
	"sal eax, 31",\
	"idiv ebx",\
	parm [eax][ebx]\
	modify [edx]\

#pragma aux divscale32 =\
	"xor eax, eax",\
	"idiv ebx",\
	parm [edx][ebx]\

#pragma aux int5 =\
	"int 0x5",\

#pragma aux setvmode =\
	"int 0x10",\
	parm [eax]\

#pragma aux setupmouse =\
	"mov ax, 0",\
	"int 33h",\
	"and eax, 0x0000ffff",\
	modify [eax]\

#pragma aux readmousexy =\
	"mov ax, 11d",\
	"int 33h",\
	"mov [esi], cx",\
	"mov [edi], dx",\
	parm [esi][edi]\
	modify [eax ebx ecx edx]\

#pragma aux readmousebstatus =\
	"mov ax, 5d",\
	"int 33h",\
	"mov [esi], ax",\
	parm [esi]\
	modify [eax ebx ecx edx]\

#pragma aux readpixel =\
	"xor eax, eax",\
	"mov al, [edi]",\
	parm [edi]\

#pragma aux drawpixel =\
	"mov byte ptr [edi], al",\
	parm [edi][eax]\

#pragma aux drawpixels =\
	"mov word ptr [edi], ax",\
	parm [edi][eax]\

#pragma aux drawpixeles =\
	"mov dword ptr [edi], eax",\
	parm [edi][eax]\

#pragma aux clearbuf =\
	"rep stosd",\
	parm [edi][ecx][eax]\

#pragma aux clearbufbyte =\
	"shr ecx, 1",\
	"jnc skip1",\
	"stosb",\
	"skip1: shr ecx, 1",\
	"jnc skip2",\
	"stosw",\
	"skip2: test ecx, ecx",\
	"jz skip3",\
	"rep stosd",\
	"skip3:",\
	parm [edi][ecx][eax]\

#pragma aux copybuf =\
	"rep movsd",\
	parm [esi][edi][ecx]\

#pragma aux copybufbyte =\
	"shr ecx, 1",\
	"jnc skip1",\
	"movsb",\
	"skip1: shr ecx, 1",\
	"jnc skip2",\
	"movsw",\
	"skip2: test ecx, ecx",\
	"jz skip3",\
	"rep movsd",\
	"skip3:",\
	parm [esi][edi][ecx]\

#pragma aux qinterpolate =\
	"inc ecx",\
	"mov ebx, ecx",\
	"shr ecx, 1",\
	"jecxz skipbegcalc",\
	"begqcalc: mov dword ptr [eax], edx",\
	"add edx, esi",\
	"mov dword ptr [eax+4], edx",\
	"add edx, esi",\
	"add eax, 8",\
	"loop begqcalc",\
	"test ebx, 1",\
	"jz skipbegqcalc2",\
	"skipbegcalc: mov dword ptr [eax], edx",\
	"skipbegqcalc2:",\
	parm [eax][ecx][edx][esi]\
	modify [ebx]\

#pragma aux qinterpolatedown16 =\
	"inc ecx",\
	"mov ebx, ecx",\
	"shr ecx, 1",\
	"jecxz skipbegcalc",\
	"begqcalc: mov edi, edx",\
	"sar edi, 16",\
	"mov dword ptr [eax], edi",\
	"add edx, esi",\
	"mov edi, edx",\
	"sar edi, 16",\
	"mov dword ptr [eax+4], edi",\
	"add edx, esi",\
	"add eax, 8",\
	"loop begqcalc",\
	"test ebx, 1",\
	"jz skipbegqcalc2",\
	"skipbegcalc: sar edx, 16",\
	"mov dword ptr [eax], edx",\
	"skipbegqcalc2:",\
	parm [eax][ecx][edx][esi]\
	modify [ebx edi]\

#pragma aux qinterpolatedown16short =\
	"inc ecx",\
	"mov ebx, ecx",\
	"shr ecx, 1",\
	"test ecx, ecx",\
	"jz skipbegcalc",\
	"begqcalc: mov edi, edx",\
	"sar edi, 16",\
	"mov word ptr [eax], di",\
	"add edx, esi",\
	"mov edi, edx",\
	"sar edi, 16",\
	"mov word ptr [eax+2], di",\
	"add edx, esi",\
	"add eax, 4",\
	"dec ecx",\
	"jnz begqcalc",\
	"test ebx, 1",\
	"jz skipbegqcalc2",\
	"skipbegcalc: sar edx, 16",\
	"mov word ptr [eax], dx",\
	"skipbegqcalc2:",\
	parm [eax][ecx][edx][esi]\
	modify [ebx edi]\

#pragma aux groudat =\
	"shl eax, 6",\
	"or eax, ebx",\
	"mov al, byte ptr [eax+ecx]",\
	"and eax, 0x000000ff",\
	parm [eax][ebx][ecx]\

#pragma aux drawslab =\
	"startdraw: mov [edi], al",\
	"sub edi, ebx",\
	"dec ecx",\
	"jnz startdraw",\
	"mov eax, edi",\
	parm [edi][ecx][eax][ebx]\

#pragma aux setcolor16 =\
	"mov dx, 0x3ce",\
	"shl ax, 8",\
	"out dx, ax",\
	parm [eax]\

#pragma aux vlin16first =\
	"mov al, byte ptr [edi]",\
	"mov eax, ecx",\
	"shr ecx, 2",\
	"begvlin16firsta: mov byte ptr [edi], al",\
	"mov byte ptr [edi+80], al",\
	"mov byte ptr [edi+160], al",\
	"mov byte ptr [edi+240], al",\
	"add edi, 320",\
	"loop begvlin16firsta",\
	"mov ecx, eax",\
	"and ecx, 3",\
	"jz skipfirst",\
	"begvlin16firstb: mov byte ptr [edi], al",\
	"add edi, 80",\
	"loop begvlin16firstb",\
	"skipfirst:",\
	parm [edi][ecx]\
	modify [eax esi]\

#pragma aux vlin16 =\
	"mov esi, edi",\
	"begvlin16: movsb",\
	"add edi, 79",\
	"add esi, 79",\
	"loop begvlin16",\
	parm [edi][ecx]\
	modify [eax esi]\

#pragma aux drawpixel16 =\
	"mov ecx, edi",\
	"mov eax, 0x00008008",\
	"mov dx, 0x3ce",\
	"ror ah, cl",\
	"shr edi, 3",\
	"out dx, ax",\
	"mov cl, byte ptr [edi+0xa0000]",\
	"mov byte ptr [edi+0xa0000], al",\
	parm [edi]\
	modify [eax ecx edx esi]\
	//IF ESI IS NOT IN THE MODIFY LINE, IT WILL SCREW UP, BUT WHY?

#pragma aux fillscreen16 =\
	"mov dx, 0x3ce",\
	"shl ax, 8",\
	"out dx, ax",\
	"mov ax, 0xff08",\
	"out dx, ax",\
	"shr ecx, 5",\
	"add edi, 0xa0000",\
	"rep stosd",\
	parm [edi][eax][ecx]\
	modify [edx]\

#pragma aux koutp =\
	"out dx, al",\
	parm [edx][eax]\

#pragma aux koutpw =\
	"out dx, ax",\
	parm [edx][eax]\

#pragma aux kinp =\
	"in al, dx",\
	parm [edx]\

#pragma aux mul80 =\
	"lea eax, [eax+eax*4]",\
	"shl eax, 4",\
	parm [eax]\

#pragma aux mul320 =\
	"lea eax, [eax+eax*4]",\
	"shl eax, 6",\
	parm [eax]\

#pragma aux mul640 =\
	"lea eax, [eax+eax*4]",\
	"shl eax, 7",\
	parm [eax]\

#pragma aux klabs =\
	"test eax, eax",\
	"jns skipnegate",\
	"neg eax",\
	"skipnegate:",\
	parm [eax]\

#pragma aux ksgn =\
	"add ebx, ebx",\
	"sbb eax, eax",\
	"cmp eax, ebx",\
	"adc eax, 0",\
	parm [ebx]\

	//eax = (unsigned min)umin(eax,ebx)
#pragma aux umin =\
	"sub eax,ebx",\
	"sbb ecx,ecx",\
	"and eax,ecx",\
	"add eax,ebx",\
	parm [eax][ebx]\
	modify [ecx]\

	//eax = (unsigned max)umax(eax,ebx)
#pragma aux umax =\
	"sub eax,ebx",\
	"sbb ecx,ecx",\
	"not ecx",\
	"and eax,ecx",\
	"add eax,ebx",\
	parm [eax][ebx]\
	modify [ecx]\

#pragma aux limitrate =\
	"mov dx, 0x3da",\
	"wait1: in al, dx",\
	"test al, 8",\
	"jnz wait1",\
	"wait2: in al, dx",\
	"test al, 8",\
	"jz wait2",\
	modify [eax edx]\

#pragma aux readtimer =\
	"mov al, 0xd2",\
	"out 0x43, al",\
	"in al, 0x40",\
	"mov ah, al",\
	"in al, 0x40",\
	"xchg al, ah",\
	"and eax, 0x000ffff",\
	modify [eax]\

#pragma aux redblueblit = \
	"xor ecx, ecx",\
	"begblit: mov eax, dword ptr [edx+ecx]",\
	"shl eax, 4",\
	"add eax, dword ptr [ebx+ecx]",\
	"mov dword ptr [ecx+0xa0000], eax",\
	"add ecx, 4",\
	"cmp ecx, 64000",\
	"jb begblit",\
	parm [ebx][edx]\
	modify [eax ecx]\

#pragma aux chainblit =\
	"shr ecx, 1",\
	"jnc prebeg",\
	"mov al, byte ptr [esi+8]",\
	"mov ah, byte ptr [esi+12]",\
	"shl eax, 16",\
	"mov al, byte ptr [esi]",\
	"mov ah, byte ptr [esi+4]",\
	"mov dword ptr [edi], eax",\
	"add esi, 16",\
	"add edi, 4",\
	"test ecx, ecx",\
	"prebeg: jz endit",\
	"beg: mov al, byte ptr [esi+8]",\
	"mov bl, byte ptr [esi+24]",\
	"mov ah, byte ptr [esi+12]",\
	"mov bh, byte ptr [esi+28]",\
	"shl eax, 16",\
	"add edi, 8",\
	"shl ebx, 16",\
	"mov al, byte ptr [esi]",\
	"mov bl, byte ptr [esi+16]",\
	"mov ah, byte ptr [esi+4]",\
	"mov bh, byte ptr [esi+20]",\
	"add esi, 32",\
	"mov dword ptr [edi-8], eax",\
	"dec ecx",\
	"mov dword ptr [edi-4], ebx",\
	"jnz beg",\
	"endit:",\
	parm [esi][edi][ecx]\
	modify [eax ebx]\

#pragma aux swapchar =\
	"mov cl, [eax]",\
	"mov ch, [ebx]",\
	"mov [ebx], cl",\
	"mov [eax], ch",\
	parm [eax][ebx]\
	modify [ecx edx]\

	//swapchar2(ptr1,ptr2,xsiz); is the same as:
	//swapchar(ptr1,ptr2); swapchar(ptr1+1,ptr2+xsiz);
#pragma aux swapchar2 =\
	"add esi, ebx",\
	"mov cx, [eax]",\
	"mov dl, [ebx]",\
	"mov [ebx], cl",\
	"mov dh, [esi]",\
	"mov [esi], ch",\
	"mov [eax], dx",\
	parm [eax][ebx][esi]\
	modify [ecx edx]\
