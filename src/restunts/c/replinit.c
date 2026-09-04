#include "restunts.h"
#include "memmgr.h"

void init_video_geometry_flags(void)
{
	video_flag1_is1 = 1;
	video_flag2_is1 = 1;
	video_flag3_isFFFF = -1;
	video_flag4_is1 = 1;
}

void init_row_tables(void)
{
	legacy_s16 i;
	legacy_s16 inverse_row;
	legacy_s16 track_position;
	legacy_s16 terrain_position;

	for (i = 0; i < 30; i++) {
		inverse_row = LEGACY_S16_WRAP_SUB(29, i);
		track_position = LEGACY_S16_SHL(inverse_row, 10U);
		terrain_position = LEGACY_S16_SHL(i, 10U);
		trackrows[i] = LEGACY_S16_WRAP_MUL(30, inverse_row);
		terrainrows[i] = LEGACY_S16_WRAP_MUL(30, i);
		trackpos[i] = track_position;
		trackpos2[i] = terrain_position;
		trackcenterpos[i] = LEGACY_S16_WRAP_ADD(
			track_position, 0x200);
		terrainpos[i] = terrain_position;
		terraincenterpos[i] = LEGACY_S16_WRAP_ADD(
			terrain_position, 0x200);
		trackcenterpos2[i] = LEGACY_S16_WRAP_ADD(
			terrain_position, 0x200);
	}
}

void init_trackdata(void)
{
	legacy_s8 far* trkptr;

	trkptr = mmgr_alloc_resbytes("trakdata", 0x6BF3);
	td01_track_file_cpy = (legacy_s16 far*)trkptr;
	trkptr += 0x70A;
	td02_penalty_related = (legacy_s16 far*)trkptr;
	trkptr += 0x70A;
	trackdata3 = trkptr;
	trkptr += 0x70A;
	td04_aerotable_pl = (legacy_s16 far*)trkptr;
	trkptr += 0x80;
	td05_aerotable_op = (legacy_s16 far*)trkptr;
	trkptr += 0x80;
	trackdata6 = (legacy_s16 far*)trkptr;
	trkptr += 0x80;
	trackdata7 = (legacy_s16 far*)trkptr;
	trkptr += 0x80;
	td08_direction_related = (legacy_s16 far*)trkptr;
	trkptr += 0x60;
	trackdata9 = (struct VECTOR far*)trkptr;
	trkptr += 0x180;
	td10_track_check_rel = (struct VECTOR far*)trkptr;
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
