#include "externs.h"

/* Exact resource identifiers and messages formerly stored in dseg.asm. */

legacy_s8 a0[4] = {
	' ', ' ', '0', 0
};

legacy_s8 a02040[10] = {
	'0', ' ', ' ', '2', '0', ' ', ' ', '4', '0', 0
};

legacy_s8 a100[4] = {
	'1', '0', '0', 0
};

legacy_s8 a150[4] = {
	'1', '5', '0', 0
};

legacy_s8 a50[4] = {
	' ', '5', '0', 0
};

legacy_s8 aArowarrwarw1ar[45] = {
	'a', 'r', 'o', 'w', 'a', 'r', 'r', 'w', 'a', 'r', 'w', '1',
	'a', 'r', 'w', '2', 'a', 'r', 'w', '3', 'a', 'r', 'w', '4',
	'a', 'r', 'w', '5', 'a', 'r', 'w', '6', 'a', 'r', 'w', '7',
	'a', 'r', 'w', '8', 't', 'y', 'p', 'e', 0
};

legacy_s8 aArt[4] = {
	'a', 'r', 't', 0
};

legacy_s8 aAvs[4] = {
	'a', 'v', 's', 0
};

/*
The name aBarn came from the disassembler naming the address after its first
string, "barn". It obscures its real purpose; something like game_shape_names
would be much clearer. The table contains track and scenery shape identifiers
such as bridges, tunnels, roads, trees, barriers, and explosion shapes.
*/
legacy_s8 aBarn[580] = {
	'b', 'a', 'r', 'n', 0, 'z', 'b', 'r', 'n', 0, 'b', 'r',
	'i', 'd', 0, 'z', 'b', 'r', 'i', 0, 'b', 't', 'u', 'r',
	0, 'z', 'b', 't', 'u', 0, 'c', 'h', 'i', '1', 0, 'z',
	'c', 'h', '1', 0, 'c', 'h', 'i', '2', 0, 'z', 'c', 'h',
	'2', 0, 'e', 'l', 'r', 'd', 0, 'z', 'e', 'l', 'r', 0,
	'f', 'i', 'n', 'i', 0, 'z', 'f', 'i', 'n', 0, 'g', 'a',
	's', 's', 0, 'z', 'g', 'a', 's', 0, 'l', 'b', 'a', 'n',
	0, 'z', 'l', 'b', 'a', 0, 'l', 'o', 'o', 'p', 0, 'z',
	'l', 'o', 'o', 0, 'o', 'f', 'f', 'i', 0, 'z', 'o', 'f',
	'f', 0, 'p', 'i', 'p', 'e', 0, 'z', 'p', 'i', 'p', 0,
	'r', 'a', 'm', 'p', 0, 'z', 'r', 'a', 'm', 0, 'r', 'b',
	'a', 'n', 0, 'z', 'r', 'b', 'a', 0, 'r', 'd', 'u', 'p',
	0, 'z', 'r', 'd', 'u', 0, 'r', 'o', 'a', 'd', 0, 'z',
	'r', 'o', 'a', 0, 's', 't', 'u', 'r', 0, 'z', 's', 't',
	'u', 0, 't', 'e', 'n', 'n', 0, 'z', 't', 'e', 'n', 0,
	't', 'u', 'n', 'n', 0, 'z', 't', 'u', 'n', 0, 't', 'u',
	'r', 'n', 0, 'z', 't', 'u', 'r', 0, 'g', 'o', 'u', 'i',
	0, 'g', 'o', 'u', 'o', 0, 'g', 'o', 'u', 'p', 0, 'h',
	'i', 'g', 'h', 0, 'l', 'a', 'k', 'c', 0, 'l', 'a', 'k',
	'e', 0, 'c', 'l', 'd', '1', 0, 'c', 'l', 'd', '2', 0,
	'c', 'l', 'd', '3', 0, 's', 'i', 'g', 'l', 0, 's', 'i',
	'g', 'r', 0, 't', 'r', 'e', 'e', 0, 'i', 'n', 't', 'e',
	0, 'z', 'i', 'n', 't', 0, 'o', 'f', 'f', 'l', 0, 'z',
	'o', 'f', 'l', 0, 'o', 'f', 'f', 'r', 0, 'z', 'o', 'f',
	'r', 0, 'p', 'a', 'l', 'm', 0, 'z', 'p', 'a', 'l', 0,
	'b', 'a', 'n', 'k', 0, 'z', 'b', 'a', 'n', 0, 's', 'o',
	'f', 'l', 0, 'z', 's', 'o', 'l', 0, 's', 'o', 'f', 'r',
	0, 'z', 's', 'o', 'r', 0, 's', 'r', 'a', 'm', 0, 'z',
	's', 'r', 'a', 0, 's', 'e', 'l', 'r', 0, 'z', 's', 'e',
	'r', 0, 'e', 'l', 's', 'p', 0, 'z', 'e', 's', 'p', 0,
	'c', 'a', 'c', 't', 0, 'c', 'a', 'c', 't', 0, 's', 'p',
	'i', 'p', 0, 'z', 's', 'p', 'i', 0, 's', 'e', 's', 't',
	0, 'z', 's', 'e', 's', 0, 'w', 'r', 'o', 'a', 0, 'z',
	'w', 'r', 'o', 0, 'b', 'a', 'r', 'r', 0, 'z', 'b', 'a',
	'r', 0, 'l', 'c', 'o', '0', 0, 'z', 'l', 'c', 'o', 0,
	'r', 'c', 'o', '0', 0, 'z', 'r', 'c', 'o', 0, 'g', 'w',
	'r', 'o', 0, 'z', 'g', 'w', 'r', 0, 'l', 'c', 'o', '1',
	0, 'r', 'c', 'o', '1', 0, 'l', 'o', 'o', '1', 0, 'h',
	'i', 'g', '1', 0, 'h', 'i', 'g', '2', 0, 'h', 'i', 'g',
	'3', 0, 'w', 'i', 'n', 'd', 0, 'z', 'w', 'i', 'n', 0,
	'b', 'o', 'a', 't', 0, 'z', 'b', 'o', 'a', 0, 'r', 'e',
	's', 't', 0, 'z', 'r', 'e', 's', 0, 'h', 'p', 'i', 'p',
	0, 'z', 'h', 'p', 'i', 0, 'v', 'c', 'o', 'r', 0, 'z',
	'v', 'c', 'o', 0, 't', 'u', 'n', '2', 0, 'p', 'i', 'p',
	'2', 0, 'f', 'e', 'n', 'c', 0, 'z', 'f', 'e', 'n', 0,
	'c', 'f', 'e', 'n', 0, 'z', 'c', 'f', 'e', 0, 'f', 'l',
	'a', 'g', 0, 't', 'r', 'u', 'k', 0, 'e', 'x', 'p', '0',
	0, 'e', 'x', 'p', '1', 0, 'e', 'x', 'p', '2', 0, 'e',
	'x', 'p', '3', 0
};

legacy_s8 aBau[4] = {
	'b', 'a', 'u', 0
};

legacy_s8 aBau_0[4] = {
	'b', 'a', 'u', 0
};

legacy_s8 aBca[4] = {
	'b', 'c', 'a', 0
};

legacy_s8 aBcl[4] = {
	'b', 'c', 'l', 0
};

legacy_s8 aBco[4] = {
	'b', 'c', 'o', 0
};

legacy_s8 aBct[4] = {
	'b', 'c', 't', 0
};

legacy_s8 aBdo[4] = {
	'b', 'd', 'o', 0
};

legacy_s8 aBdo_0[4] = {
	'b', 'd', 'o', 0
};

legacy_s8 aBdr[4] = {
	'b', 'd', 'r', 0
};

legacy_s8 aBev[4] = {
	'b', 'e', 'v', 0
};

legacy_s8 aBhi[4] = {
	'b', 'h', 'i', 0
};

legacy_s8 aBla[4] = {
	'b', 'l', 'a', 0
};

legacy_s8 aBla_0[4] = {
	'b', 'l', 'a', 0
};

legacy_s8 aBma[4] = {
	'b', 'm', 'a', 0
};

legacy_s8 aBma_0[4] = {
	'b', 'm', 'a', 0
};

legacy_s8 aBmm_0[60] = {
	'b', 'm', 'm', 0, 2, 0, 1, 0, 2, 0, 3, 0,
	4, 0, 1, 0, 4, 0, 0, 0, 5, 0, 0, 0,
	0, 0, 6, 0, 5, 0, 6, 0, 5, 0, 1, 0,
	1, 0, 2, 0, 3, 0, 5, 0, 0, 0, 6, 0,
	2, 0, 3, 0, 4, 0, 4, 0, 0, 0, 6, 0
};

legacy_s8 aBnx[4] = {
	'b', 'n', 'x', 0
};

legacy_s8 aBnx_0[4] = {
	'b', 'n', 'x', 0
};

legacy_s8 aBra[4] = {
	'b', 'r', 'a', 0
};

legacy_s8 aBrp[4] = {
	'b', 'r', 'p', 0
};

legacy_s8 aCar[5] = {
	'c', 'a', 'r', '*', 0
};

legacy_s8 aClip[5] = {
	'c', 'l', 'i', 'p', 0
};

legacy_s8 aCon[4] = {
	'c', 'o', 'n', 0
};

legacy_s8 aCon_0[4] = {
	'c', 'o', 'n', 0
};

legacy_s8 aCre[4] = {
	'c', 'r', 'e', 0
};

legacy_s8 aCred[5] = {
	'c', 'r', 'e', 'd', 0
};

legacy_s8 aD4a[4] = {
	'd', '4', 'a', 0
};

legacy_s8 aDash[5] = {
	'd', 'a', 's', 'h', 0
};

legacy_s8 aDasm[5] = {
	'd', 'a', 's', 'm', 0
};

legacy_s8 aDast[5] = {
	'd', 'a', 's', 't', 0
};

legacy_s8 aDea[4] = {
	'd', 'e', 'a', 0
};

legacy_s8 aDefault_1[10] = {
	'D', 'E', 'F', 'A', 'U', 'L', 'T', 0, 0, 0
};

legacy_s8 aDer[4] = {
	'd', 'e', 'r', 0
};

legacy_s8 aDes[4] = {
	'd', 'e', 's', 0
};

legacy_s8 aDes_0[4] = {
	'd', 'e', 's', 0
};

legacy_s8 aDes_1[4] = {
	'd', 'e', 's', 0
};

legacy_s8 aDig0dig1dig2dig3dig4dig5d[41] = {
	'd', 'i', 'g', '0', 'd', 'i', 'g', '1', 'd', 'i', 'g', '2',
	'd', 'i', 'g', '3', 'd', 'i', 'g', '4', 'd', 'i', 'g', '5',
	'd', 'i', 'g', '6', 'd', 'i', 'g', '7', 'd', 'i', 'g', '8',
	'd', 'i', 'g', '9', 0
};

legacy_s8 aDnf[4] = {
	'd', 'n', 'f', 0
};

legacy_s8 aDnf_0[4] = {
	'd', 'n', 'f', 0
};

legacy_s8 aDos_0[4] = {
	'd', 'o', 's', 0
};

legacy_s8 aElt[4] = {
	'e', 'l', 't', 0
};

const legacy_s8 aExitListOverflow[22] = {
	'E', 'X', 'I', 'T', ' ', 'L', 'I', 'S', 'T', ' ', 'O', 'V',
	'E', 'R', 'F', 'L', 'O', 'W', 13, 10, 0, 0
};

legacy_s8 aFex_0[4] = {
	'f', 'e', 'x', 0
};

legacy_s8 aGbra[5] = {
	'g', 'b', 'r', 'a', 0
};

legacy_s8 aGbra_0[5] = {
	'g', 'b', 'r', 'a', 0
};

legacy_s8 aGbri[5] = {
	'g', 'b', 'r', 'i', 0
};

legacy_s8 aGdav[5] = {
	'g', 'd', 'a', 'v', 0
};

legacy_s8 aGdon[5] = {
	'g', 'd', 'o', 'n', 0
};

legacy_s8 aGds0[5] = {
	'g', 'd', 's', '0', 0
};

legacy_s8 aGds1[5] = {
	'g', 'd', 's', '1', 0
};

legacy_s8 aGkev[5] = {
	'g', 'k', 'e', 'v', 0
};

legacy_s8 aGkev_0[5] = {
	'g', 'k', 'e', 'v', 0
};

legacy_s8 aGkev_1[5] = {
	'g', 'k', 'e', 'v', 0
};

legacy_s8 aGkri[5] = {
	'g', 'k', 'r', 'i', 0
};

legacy_s8 aGmsm[5] = {
	'g', 'm', 's', 'm', 0
};

legacy_s8 aGmsy[5] = {
	'g', 'm', 's', 'y', 0
};

legacy_s8 aGnic[5] = {
	'g', 'n', 'i', 'c', 0
};

legacy_s8 aGnobgnabdotDotadot1dot2[25] = {
	'g', 'n', 'o', 'b', 'g', 'n', 'a', 'b', 'd', 'o', 't', ' ',
	'd', 'o', 't', 'a', 'd', 'o', 't', '1', 'd', 'o', 't', '2',
	0
};

legacy_s8 aGrap[5] = {
	'g', 'r', 'a', 'p', 0
};

legacy_s8 aGric[5] = {
	'g', 'r', 'i', 'c', 0
};

legacy_s8 aGrob[5] = {
	'g', 'r', 'o', 'b', 0
};

legacy_s8 aGsta[5] = {
	'g', 's', 't', 'a', 0
};

legacy_s8 aHna[4] = {
	'h', 'n', 'a', 0
};

legacy_s8 aId1[4] = {
	0x69, 0x64, 0x31, 0x00
};

legacy_s8 aId2[4] = {
	0x69, 0x64, 0x32, 0x00
};

legacy_s8 aId3[4] = {
	0x69, 0x64, 0x33, 0x00
};

legacy_s8 aId4[4] = {
	0x69, 0x64, 0x34, 0x00
};

legacy_s8 aIhd[4] = {
	0x69, 0x68, 0x64, 0x00
};

legacy_s8 aImp[4] = {
	0x69, 0x6D, 0x70, 0x00
};

legacy_s8 aInh[4] = {
	0x69, 0x6E, 0x68, 0x00
};

legacy_s8 aInh_0[4] = {
	0x69, 0x6E, 0x68, 0x00
};

legacy_s8 aJum[4] = {
	0x6A, 0x75, 0x6D, 0x00
};

legacy_s8 aKey[4] = {
	0x6B, 0x65, 0x79, 0x00
};

legacy_s8 aLoa[4] = {
	0x6C, 0x6F, 0x61, 0x00
};

legacy_s8 aLose[5] = {
	0x6C, 0x6F, 0x73, 0x65, 0x00
};

legacy_s8 aLsd[4] = {
	0x6C, 0x73, 0x64, 0x00
};

legacy_s8 aLsu[4] = {
	0x6C, 0x73, 0x75, 0x00
};

legacy_s8 aMdo[4] = {
	0x6D, 0x64, 0x6F, 0x00
};

legacy_s8 aMen_0[4] = {
	0x6D, 0x65, 0x6E, 0x00
};

legacy_s8 aMer[4] = {
	0x6D, 0x65, 0x72, 0x00
};

legacy_s8 aMisc[5] = {
	0x6D, 0x69, 0x73, 0x63, 0x00
};

legacy_s8 aMisc_0[5] = {
	0x6D, 0x69, 0x73, 0x63, 0x00
};

legacy_s8 aMisc_1[5] = {
	0x6D, 0x69, 0x73, 0x63, 0x00
};

legacy_s8 aMisc_2[5] = {
	0x6D, 0x69, 0x73, 0x63, 0x00
};

legacy_s8 aMof[4] = {
	0x6D, 0x6F, 0x66, 0x00
};

legacy_s8 aMon[4] = {
	0x6D, 0x6F, 0x6E, 0x00
};

legacy_s8 aMou[4] = {
	0x6D, 0x6F, 0x75, 0x00
};

legacy_s8 aMph[4] = {
	0x6D, 0x70, 0x68, 0x00
};

legacy_s8 aMph_0[4] = {
	0x6D, 0x70, 0x68, 0x00
};

legacy_s8 aMph_1[4] = {
	0x6D, 0x70, 0x68, 0x00
};

legacy_s8 aMrl[4] = {
	0x6D, 0x72, 0x6C, 0x00
};

legacy_s8 aMrs[4] = {
	0x6D, 0x72, 0x73, 0x00
};

legacy_s8 aMus[4] = {
	0x6D, 0x75, 0x73, 0x00
};

legacy_s8 aOlt[4] = {
	0x6F, 0x6C, 0x74, 0x00
};

legacy_s8 aOlt_0[4] = {
	0x6F, 0x6C, 0x74, 0x00
};

legacy_s8 aOp01[5] = {
	0x6F, 0x70, 0x30, 0x31, 0x00
};

legacy_s8 aOpp0opp1opp2op[29] = {
	0x6F, 0x70, 0x70, 0x30, 0x6F, 0x70, 0x70, 0x31, 0x6F, 0x70, 0x70, 0x32,
	0x6F, 0x70, 0x70, 0x33, 0x6F, 0x70, 0x70, 0x34, 0x6F, 0x70, 0x70, 0x35,
	0x6F, 0x70, 0x70, 0x36, 0x00
};

legacy_s8 aOpp2lose[10] = {
	0x6F, 0x70, 0x70, 0x32, 0x6C, 0x6F, 0x73, 0x65, 0x00, 0x00
};

legacy_s8 aOpp2win[8] = {
	0x6F, 0x70, 0x70, 0x32, 0x77, 0x69, 0x6E, 0x00
};

legacy_s8 aOpr[4] = {
	0x6F, 0x70, 0x72, 0x00
};

legacy_s8 aOver[5] = {
	0x4F, 0x56, 0x45, 0x52, 0x00
};

legacy_s8 aOwt[4] = {
	0x6F, 0x77, 0x74, 0x00
};

legacy_s8 aPau[4] = {
	0x70, 0x61, 0x75, 0x00
};

legacy_s8 aPpt[4] = {
	0x70, 0x70, 0x74, 0x00
};

legacy_s8 aPro[4] = {
	0x70, 0x72, 0x6F, 0x00
};

legacy_s8 aRac[5] = {
	0x72, 0x61, 0x63, 0x00, 0x00
};

legacy_s8 aRep_1[4] = {
	0x72, 0x65, 0x70, 0x00
};

legacy_s8 aRoof[5] = {
	0x72, 0x6F, 0x6F, 0x66, 0x00
};

legacy_s8 aRplyrpicrpacrpmcrptcbof6bof5b[93] = {
	0x72, 0x70, 0x6C, 0x79, 0x72, 0x70, 0x69, 0x63, 0x72, 0x70, 0x61, 0x63,
	0x72, 0x70, 0x6D, 0x63, 0x72, 0x70, 0x74, 0x63, 0x62, 0x6F, 0x66, 0x36,
	0x62, 0x6F, 0x66, 0x35, 0x62, 0x6F, 0x66, 0x34, 0x62, 0x6F, 0x66, 0x33,
	0x62, 0x6F, 0x66, 0x32, 0x62, 0x6F, 0x66, 0x31, 0x62, 0x6F, 0x66, 0x30,
	0x7A, 0x6F, 0x6F, 0x6D, 0x70, 0x61, 0x6E, 0x6E, 0x62, 0x6F, 0x6E, 0x36,
	0x62, 0x6F, 0x6E, 0x35, 0x62, 0x6F, 0x6E, 0x34, 0x62, 0x6F, 0x6E, 0x33,
	0x62, 0x6F, 0x6E, 0x32, 0x62, 0x6F, 0x6E, 0x31, 0x62, 0x6F, 0x66, 0x30,
	0x7A, 0x6F, 0x6F, 0x6D, 0x70, 0x61, 0x6E, 0x6E, 0x00
};

const legacy_s8 aSFileError[15] = {
	0x25, 0x73, 0x20, 0x46, 0x49, 0x4C, 0x45, 0x20, 0x45, 0x52, 0x52, 0x4F,
	0x52, 0x0D, 0x00
};

const legacy_s8 aSFileError_0[16] = {
	0x25, 0x73, 0x20, 0x46, 0x49, 0x4C, 0x45, 0x20, 0x45, 0x52, 0x52, 0x4F,
	0x52, 0x0D, 0x00, 0x00
};

const legacy_s8 aSFileError_1[14] = {
	0x25, 0x73, 0x20, 0x46, 0x49, 0x4C, 0x45, 0x20, 0x45, 0x52, 0x52, 0x4F,
	0x52, 0x00
};

const legacy_s8 aSInvalidPackTy[23] = {
	0x25, 0x73, 0x20, 0x49, 0x4E, 0x56, 0x41, 0x4C, 0x49, 0x44, 0x20, 0x50,
	0x41, 0x43, 0x4B, 0x20, 0x54, 0x59, 0x50, 0x45, 0x0D, 0x00, 0x00
};

legacy_s8 aSav[5] = {
	0x73, 0x61, 0x76, 0x00, 0x00
};

legacy_s8 aScrn[18] = {
	0x73, 0x63, 0x72, 0x6E, 0x00, 0x00, 0x01, 0x02, 0x04, 0x00, 0x03, 0x00,
	0x03, 0x00, 0x01, 0x04, 0x02, 0x00
};

legacy_s8 aScrn_0[5] = {
	0x73, 0x63, 0x72, 0x6E, 0x00
};

legacy_s8 aSdcsel[7] = {
	0x73, 0x64, 0x63, 0x73, 0x65, 0x6C, 0x00
};

legacy_s8 aSdmsel[7] = {
	0x73, 0x64, 0x6D, 0x73, 0x65, 0x6C, 0x00
};

legacy_s8 aSdosel[7] = {
	0x73, 0x64, 0x6F, 0x73, 0x65, 0x6C, 0x00
};

legacy_s8 aSer_0[4] = {
	0x73, 0x65, 0x72, 0x00
};

legacy_s8 aSkidms_1[7] = {
	0x73, 0x6B, 0x69, 0x64, 0x6D, 0x73, 0x00
};

legacy_s8 aSkidms_2[7] = {
	0x73, 0x6B, 0x69, 0x64, 0x6D, 0x73, 0x00
};

legacy_s8 aSkidover[9] = {
	0x73, 0x6B, 0x69, 0x64, 0x6F, 0x76, 0x65, 0x72, 0x00
};

legacy_s8 aSkidvict[9] = {
	0x73, 0x6B, 0x69, 0x64, 0x76, 0x69, 0x63, 0x74, 0x00
};

legacy_s8 aSof[4] = {
	0x73, 0x6F, 0x66, 0x00
};

legacy_s8 aSon[4] = {
	0x73, 0x6F, 0x6E, 0x00
};

legacy_s8 aStdaxxxx[10] = {
	0x73, 0x74, 0x64, 0x61, 0x78, 0x78, 0x78, 0x78, 0x00, 0x00
};

legacy_s8 aStdbxxxx[9] = {
	0x73, 0x74, 0x64, 0x62, 0x78, 0x78, 0x78, 0x78, 0x00
};

legacy_s8 aStop_1[5] = {
	0x73, 0x74, 0x6F, 0x70, 0x00
};

legacy_s8 aStxxx[7] = {
	0x73, 0x74, 0x78, 0x78, 0x78, 0x00, 0x00
};

legacy_s8 aTop[4] = {
	0x74, 0x6F, 0x70, 0x00
};

legacy_s8 aVict[5] = {
	0x56, 0x49, 0x43, 0x54, 0x00
};

legacy_s8 aWai[4] = {
	0x77, 0x61, 0x69, 0x00
};

legacy_s8 aWhl1whl2whl3ins2gboxins1i[37] = {
	0x77, 0x68, 0x6C, 0x31, 0x77, 0x68, 0x6C, 0x32, 0x77, 0x68, 0x6C, 0x33,
	0x69, 0x6E, 0x73, 0x32, 0x67, 0x62, 0x6F, 0x78, 0x69, 0x6E, 0x73, 0x31,
	0x69, 0x6E, 0x73, 0x33, 0x69, 0x6E, 0x6D, 0x31, 0x69, 0x6E, 0x6D, 0x33,
	0x00
};

legacy_s8 aWindowReleased[32] = {
	0x57, 0x69, 0x6E, 0x64, 0x6F, 0x77, 0x20, 0x52, 0x65, 0x6C, 0x65, 0x61,
	0x73, 0x65, 0x64, 0x20, 0x4F, 0x75, 0x74, 0x20, 0x6F, 0x66, 0x20, 0x4F,
	0x72, 0x64, 0x65, 0x72, 0x0D, 0x0A, 0x00, 0x00
};

legacy_s8 aWindowdefOutOfRowTableSpa[36] = {
	0x77, 0x69, 0x6E, 0x64, 0x6F, 0x77, 0x64, 0x65, 0x66, 0x20, 0x2D, 0x20,
	0x4F, 0x55, 0x54, 0x20, 0x4F, 0x46, 0x20, 0x52, 0x4F, 0x57, 0x20, 0x54,
	0x41, 0x42, 0x4C, 0x45, 0x20, 0x53, 0x50, 0x41, 0x43, 0x45, 0x0D, 0x00
};

legacy_s8 aWinn[5] = {
	0x77, 0x69, 0x6E, 0x6E, 0x00
};

legacy_s8 a_res_0[5] = {
	0x2E, 0x72, 0x65, 0x73, 0x00
};

legacy_s8 a_rpl_2[5] = {
	0x2E, 0x72, 0x70, 0x6C, 0x00
};

legacy_s8 a_trk_5[5] = {
	0x2E, 0x74, 0x72, 0x6B, 0x00
};

legacy_s8 audiodriverstring[5] = {
	0x70, 0x63, 0x31, 0x35, 0x00
};
