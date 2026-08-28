#include "restunts.h"
#include "memmgr.h"

void init_row_tables(void)
{
	legacy_s16 i;

	for (i = 0; i < 30; i++) {
		trackrows[i] = 30 * (29 - i);
		terrainrows[i] = 30 * i;
		trackpos[i] = (29 - i) << 10;
		trackpos2[i] = i << 10;
		trackcenterpos[i] = ((29 - i) << 10) + 0x200;
		terrainpos[i] = i << 10;
		terraincenterpos[i] = (i << 10) + 0x200;
		trackcenterpos2[i] = (i << 10) + 0x200;
	}
}

void init_trackdata(void)
{
	legacy_s8 far* trkptr;

	trkptr = mmgr_alloc_resbytes("trakdata", 0x6BF3);
	td01_track_file_cpy = trkptr;
	trkptr += 0x70A;
	td02_penalty_related = trkptr;
	trkptr += 0x70A;
	trackdata3 = trkptr;
	trkptr += 0x70A;
	td04_aerotable_pl = trkptr;
	trkptr += 0x80;
	td05_aerotable_op = trkptr;
	trkptr += 0x80;
	trackdata6 = trkptr;
	trkptr += 0x80;
	trackdata7 = trkptr;
	trkptr += 0x80;
	td08_direction_related = trkptr;
	trkptr += 0x60;
	trackdata9 = trkptr;
	trkptr += 0x180;
	td10_track_check_rel = trkptr;
	trkptr += 0x120;
	td11_highscores = trkptr;
	trkptr += 0x16C;
	trackdata12 = trkptr;
	trkptr += 0xF0;
	td13_rpl_header = trkptr;
	trkptr += 0x1A;
	td14_elem_map_main = trkptr;
	trkptr += 0x385;
	td15_terr_map_main = trkptr;
	trkptr += 0x385;
	td16_rpl_buffer = trkptr;
	trkptr += 0x2EE0;
	td17_trk_elem_ordered = trkptr;
	trkptr += 0x385;
	trackdata18 = trkptr;
	trkptr += 0x385;
	trackdata19 = trkptr;
	trkptr += 0x385;
	td20_trk_file_appnd = trkptr;
	trkptr += 0x7AC;
	td21_col_from_path = trkptr;
	trkptr += 0x385;
	td22_row_from_path = trkptr;
	trkptr += 0x385;
	trackdata23 = trkptr;
	trkptr += 0x30;
}

void init_unknown(void)
{
	byte_44A8A = 1;
	byte_4552F = 2;
	elapsed_time2 = 0;
	byte_449DA = 0;
	byte_4393C = 0;
	word_44DCA = 0;
}
