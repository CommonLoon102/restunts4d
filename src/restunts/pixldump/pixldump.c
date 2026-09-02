#include <restunts.h>
#include <fileio.h>
#include <memmgr.h>
#include <platform.h>
#include <shape2d.h>
#include <shape3d.h>

#include "md5.h"

#define PIXLDUMP_FRAMEBUFFER_SIZE 64000U
#define PIXLDUMP_SAMPLE_INTERVAL 5U
#define PIXLDUMP_SCREEN_WIDTH 320U
#define PIXLDUMP_SCREEN_HEIGHT 200U
#define PIXLDUMP_BMP_HEADER_SIZE 54U
#define PIXLDUMP_BMP_PALETTE_SIZE 1024U
#define PIXLDUMP_BMP_PIXEL_OFFSET 1078UL
#define PIXLDUMP_BMP_FILE_SIZE 65078UL
#define PIXLDUMP_OUTPUT_NAME_SIZE 128U

#ifdef RESTUNTS_ORIGINAL
#define PIXLDUMP_DUMP_EXTENSION ".PDO"
#define PIXLDUMP_PROGRAM_NAME "PIXLDUMO"
#else
#define PIXLDUMP_DUMP_EXTENSION ".PDD"
#define PIXLDUMP_PROGRAM_NAME "PIXLDUMP"
#endif

typedef legacy_u16 PIXLDUMP_OUTPUT;

static PIXLDUMP_OUTPUT pixldump_output_open(const legacy_s8* path)
{
	return dos_file_open(path, 1);
}

static legacy_u16 pixldump_output_write(PIXLDUMP_OUTPUT output,
	const void far* source, legacy_u16 length)
{
	return dos_file_write(output, source, length);
}

static void pixldump_output_close(PIXLDUMP_OUTPUT output)
{
	(void)dos_file_close(output);
}

static void pixldump_write_stdout(const legacy_s8* message)
{
	(void)pixldump_output_write(1U, message, strlen(message));
}

static legacy_s16 pixldump_parse_camera(const legacy_s8* argument)
{
	if (argument[0] < '1' || argument[0] > '4' || argument[1] != 0)
		return 0;
	return (legacy_s16)(argument[0] - '0');
}

static legacy_s16 pixldump_parse_target(const legacy_s8* argument)
{
	if ((argument[0] != '0' && argument[0] != '1') || argument[1] != 0)
		return -1;
	return (legacy_s16)(argument[0] - '0');
}

static legacy_s16 pixldump_parse_frame(const legacy_s8* argument,
	legacy_u16* frame)
{
	legacy_u32 value;
	legacy_u16 index;

	if (argument[0] == 0)
		return 0;
	value = 0;
	for (index = 0; argument[index] != 0; index++) {
		if (argument[index] < '0' || argument[index] > '9')
			return 0;
		value = value * 10UL + (legacy_u32)(argument[index] - '0');
		if (value > 0xFFFFUL)
			return 0;
	}
	*frame = (legacy_u16)value;
	return 1;
}

static void pixldump_write_usage(void)
{
	pixldump_write_stdout("Usage:\r\n");
	pixldump_write_stdout(
		"  " PIXLDUMP_PROGRAM_NAME " REPLNAME CAMERA TARGET\r\n");
	pixldump_write_stdout(
		"  " PIXLDUMP_PROGRAM_NAME
		" REPLNAME CAMERA TARGET FRAME\r\n");
	pixldump_write_stdout("CAMERA: 1=F1, 2=F2, 3=F3, 4=F4\r\n");
	pixldump_write_stdout("TARGET: 0=player, 1=opponent\r\n");
}

#ifdef RESTUNTS_ORIGINAL
void init_row_tables(void)
{
	legacy_s16 index;

	for (index = 0; index < 30; index++) {
		trackrows[index] = 30 * (29 - index);
		terrainrows[index] = 30 * index;
		trackpos[index] = (29 - index) << 10;
		trackcenterpos[index] = ((29 - index) << 10) + 0x200;
		terrainpos[index] = index << 10;
		terraincenterpos[index] = (index << 10) + 0x200;
	}

	for (index = 0; index < 30; index++) {
		trackpos2[index] = index << 10;
		trackcenterpos2[index] = (index << 10) + 0x200;
	}
}

void init_trackdata(void)
{
	legacy_s8 far* track_pointer;

	track_pointer = mmgr_alloc_resbytes("trakdata", 0x6BF3);
	td01_track_file_cpy = (legacy_s16 far*)track_pointer;
	track_pointer += 0x70A;
	td02_penalty_related = (legacy_s16 far*)track_pointer;
	track_pointer += 0x70A;
	trackdata3 = track_pointer;
	track_pointer += 0x70A;
	td04_aerotable_pl = (legacy_s16 far*)track_pointer;
	track_pointer += 0x80;
	td05_aerotable_op = (legacy_s16 far*)track_pointer;
	track_pointer += 0x80;
	trackdata6 = (legacy_s16 far*)track_pointer;
	track_pointer += 0x80;
	trackdata7 = (legacy_s16 far*)track_pointer;
	track_pointer += 0x80;
	td08_direction_related = (legacy_s16 far*)track_pointer;
	track_pointer += 0x60;
	trackdata9 = (legacy_s16 far*)track_pointer;
	track_pointer += 0x180;
	td10_track_check_rel = (legacy_s16 far*)track_pointer;
	track_pointer += 0x120;
	td11_highscores = track_pointer;
	track_pointer += 0x16C;
	trackdata12 = track_pointer;
	track_pointer += 0x0F0;
	td13_rpl_header = track_pointer;
	track_pointer += 0x1A;
	td14_elem_map_main = (legacy_u8 far*)track_pointer;
	track_pointer += 0x385;
	td15_terr_map_main = (legacy_u8 far*)track_pointer;
	track_pointer += 0x385;
	td16_rpl_buffer = track_pointer;
	track_pointer += 0x2EE0;
	td17_trk_elem_ordered = track_pointer;
	track_pointer += 0x385;
	trackdata18 = track_pointer;
	track_pointer += 0x385;
	trackdata19 = (legacy_u8 far*)track_pointer;
	track_pointer += 0x385;
	td20_trk_file_appnd = track_pointer;
	track_pointer += 0x7AC;
	td21_col_from_path = track_pointer;
	track_pointer += 0x385;
	td22_row_from_path = track_pointer;
	track_pointer += 0x385;
	trackdata23 = (legacy_u8 far*)track_pointer;
}
#endif

static legacy_u16 pixldump_append_frame_number(legacy_s8* line,
	legacy_u16 frame)
{
	legacy_s8 reversed[5];
	legacy_u16 count;
	legacy_u16 index;

	count = 0;
	do {
		reversed[count++] = (legacy_s8)('0' + frame % 10U);
		frame = (legacy_u16)(frame / 10U);
	} while (frame != 0U);

	for (index = 0; index < count; index++)
		line[index] = reversed[count - index - 1U];
	return count;
}

static legacy_u16 pixldump_frame_number_length(legacy_u16 frame)
{
	legacy_u16 length;

	length = 1;
	while (frame >= 10U) {
		frame = (legacy_u16)(frame / 10U);
		length++;
	}
	return length;
}

static legacy_s16 pixldump_build_output_name(legacy_s8* output_name,
	const legacy_s8* replay_name, legacy_s16 camera_number,
	legacy_s16 target, legacy_s16 bmp_mode, legacy_u16 frame)
{
	legacy_u16 length;
	legacy_u16 required;

	length = strlen(replay_name);
	if (bmp_mode != 0) {
		required = (legacy_u16)(5U +
			pixldump_frame_number_length(frame) +
			strlen(PIXLDUMP_DUMP_EXTENSION ".bmp"));
	} else {
		required = strlen(PIXLDUMP_DUMP_EXTENSION);
	}
	if (length + required >= PIXLDUMP_OUTPUT_NAME_SIZE)
		return 0;

	strcpy(output_name, replay_name);
	if (bmp_mode != 0) {
		output_name[length++] = '.';
		output_name[length++] = (legacy_s8)('0' + camera_number);
		output_name[length++] = '.';
		output_name[length++] = (legacy_s8)('0' + target);
		output_name[length++] = '.';
		length += pixldump_append_frame_number(output_name + length, frame);
		strcpy(output_name + length, PIXLDUMP_DUMP_EXTENSION ".bmp");
	} else {
		strcat(output_name, PIXLDUMP_DUMP_EXTENSION);
	}
	return 1;
}

static legacy_s16 pixldump_write_sample(PIXLDUMP_OUTPUT output,
	legacy_u16 frame, const legacy_u8 far* framebuffer)
{
	static const legacy_s8 hex_digits[] = "0123456789abcdef";
	legacy_u8 digest[PIXLDUMP_MD5_SIZE];
	legacy_s8 line[40];
	legacy_u16 line_length;
	legacy_u16 index;

	pixldump_md5(framebuffer, PIXLDUMP_FRAMEBUFFER_SIZE, digest);
	line_length = pixldump_append_frame_number(line, frame);
	line[line_length++] = ' ';
	for (index = 0; index < PIXLDUMP_MD5_SIZE; index++) {
		line[line_length++] = hex_digits[digest[index] >> 4];
		line[line_length++] = hex_digits[digest[index] & 0x0FU];
	}
	line[line_length++] = '\r';
	line[line_length++] = '\n';
	return pixldump_output_write(output, line, line_length) == line_length;
}

static void pixldump_store_u16(legacy_u8* output, legacy_u16 value)
{
	output[0] = (legacy_u8)value;
	output[1] = (legacy_u8)(value >> 8);
}

static void pixldump_store_u32(legacy_u8* output, legacy_u32 value)
{
	output[0] = (legacy_u8)value;
	output[1] = (legacy_u8)(value >> 8);
	output[2] = (legacy_u8)(value >> 16);
	output[3] = (legacy_u8)(value >> 24);
}

static legacy_u8 pixldump_expand_palette_channel(legacy_u8 value)
{
	return (legacy_u8)((value << 2) | (value >> 4));
}

static void pixldump_load_bmp_palette(legacy_u8* bmp_palette)
{
	legacy_s8 far* resource;
	legacy_u8 far* source;
	legacy_u16 index;
	legacy_u16 source_offset;
	legacy_u16 destination_offset;

	resource = (legacy_s8 far*)file_load_shape2d_fatal("sdmain");
	source = (legacy_u8 far*)locate_shape_fatal(resource, "!pal") +
		SHAPE2D_HEADER_SIZE;
	for (index = 0; index < 256U; index++) {
		source_offset = (legacy_u16)(index * 3U);
		destination_offset = (legacy_u16)(index * 4U);
		bmp_palette[destination_offset] =
			pixldump_expand_palette_channel(source[source_offset + 2U]);
		bmp_palette[destination_offset + 1U] =
			pixldump_expand_palette_channel(source[source_offset + 1U]);
		bmp_palette[destination_offset + 2U] =
			pixldump_expand_palette_channel(source[source_offset]);
		bmp_palette[destination_offset + 3U] = 0;
	}
	(void)mmgr_free(resource);
}

static legacy_s16 pixldump_write_bmp(const legacy_s8* output_name,
	const legacy_u8 far* framebuffer)
{
	static legacy_u8 bmp_palette[PIXLDUMP_BMP_PALETTE_SIZE];
	legacy_u8 header[PIXLDUMP_BMP_HEADER_SIZE];
	const legacy_u8 far* source_row;
	legacy_u16 index;
	legacy_u16 row;
	legacy_s16 result;
	PIXLDUMP_OUTPUT output;

	for (index = 0; index < PIXLDUMP_BMP_HEADER_SIZE; index++)
		header[index] = 0;
	header[0] = 'B';
	header[1] = 'M';
	pixldump_store_u32(header + 2U, PIXLDUMP_BMP_FILE_SIZE);
	pixldump_store_u32(header + 10U, PIXLDUMP_BMP_PIXEL_OFFSET);
	pixldump_store_u32(header + 14U, 40UL);
	pixldump_store_u32(header + 18U, PIXLDUMP_SCREEN_WIDTH);
	pixldump_store_u32(header + 22U, PIXLDUMP_SCREEN_HEIGHT);
	pixldump_store_u16(header + 26U, 1U);
	pixldump_store_u16(header + 28U, 8U);
	pixldump_store_u32(header + 34U, PIXLDUMP_FRAMEBUFFER_SIZE);
	pixldump_store_u32(header + 46U, 256UL);

	pixldump_load_bmp_palette(bmp_palette);
	output = pixldump_output_open(output_name);
	if (output == 0)
		return 1;

	result = pixldump_output_write(output, header,
		PIXLDUMP_BMP_HEADER_SIZE) == PIXLDUMP_BMP_HEADER_SIZE;
	if (result != 0) {
		result = pixldump_output_write(output, bmp_palette,
			PIXLDUMP_BMP_PALETTE_SIZE) == PIXLDUMP_BMP_PALETTE_SIZE;
	}
	for (row = 0; result != 0 && row < PIXLDUMP_SCREEN_HEIGHT; row++) {
		source_row = framebuffer + (legacy_u16)(PIXLDUMP_SCREEN_WIDTH *
			(PIXLDUMP_SCREEN_HEIGHT - row - 1U));
		result = pixldump_output_write(output, source_row,
			PIXLDUMP_SCREEN_WIDTH) == PIXLDUMP_SCREEN_WIDTH;
	}

	pixldump_output_close(output);
	return result == 0;
}

static void pixldump_render_frame(void)
{
	sprite_copy_wnd_to_1();
	update_frame(0, &rect_windshield);
	sub_19F14(&rect_windshield);
	/* The normal presentation path draws the software mouse cursor last. */
	mouse_draw_opaque_check();
}

#ifndef RESTUNTS_ORIGINAL
extern void call_exitlist(void);
#else
extern void kb_shift_checking1(void);
#endif
extern legacy_s16 setup_player_cars(void);

static void pixldump_update_gamestate(void)
{
#ifdef RESTUNTS_ORIGINAL
	input_do_checking(1);
#endif
	update_gamestate();
}

static legacy_s16 pixldump_write_frames(const legacy_s8* output_name)
{
	legacy_s16 result;
	legacy_u8 far* framebuffer;
	PIXLDUMP_OUTPUT output;

	output = pixldump_output_open(output_name);
	if (output == 0)
		return 1;

	framebuffer = (legacy_u8 far*)dos_memory_make_pointer(0xA000, 0);
	pixldump_render_frame();
	result = !pixldump_write_sample(output, 0U, framebuffer);

	while (result == 0 && gameconfig.game_recordedframes >
		(legacy_u16)state.game_frame) {
		pixldump_update_gamestate();
		if ((legacy_u16)state.game_frame % PIXLDUMP_SAMPLE_INTERVAL == 0U) {
			pixldump_render_frame();
			if (!pixldump_write_sample(output,
				(legacy_u16)state.game_frame, framebuffer))
				result = 1;
		}
	}

	pixldump_output_close(output);
	return result;
}

static legacy_s16 pixldump_write_requested_frame(
	const legacy_s8* output_name, legacy_u16 requested_frame)
{
	legacy_u8 far* framebuffer;

	while ((legacy_u16)state.game_frame < requested_frame)
		pixldump_update_gamestate();
	framebuffer = (legacy_u8 far*)dos_memory_make_pointer(0xA000, 0);
	pixldump_render_frame();
	return pixldump_write_bmp(output_name, framebuffer);
}

static legacy_s16 pixldump_process_replay(const legacy_s8* replay_name,
	const legacy_s8* output_name, legacy_s16 camera_number,
	legacy_s16 target, legacy_s16 bmp_mode, legacy_u16 requested_frame)
{
	legacy_s16 index;

	if (file_load_replay("", replay_name) != 0)
		return 1;
	if (target != 0 && gameconfig.game_opponenttype == 0) {
		pixldump_write_stdout("Replay does not contain an opponent.\r\n");
		return 1;
	}
	if (bmp_mode != 0 && requested_frame >
		gameconfig.game_recordedframes) {
		pixldump_write_stdout("Requested frame is outside the replay.\r\n");
		return 1;
	}

	_memcpy(&gameconfigcopy, &gameconfig, sizeof(struct GAMEINFO));
	for (index = 0; index < 0x70A; index++)
		td20_trk_file_appnd[index] = td14_elem_map_main[index];
	for (index = 0; index < 0x51; index++) {
		td20_trk_file_appnd[index + 0x70A] = byte_3B80C[index];
		td20_trk_file_appnd[index + 0x75B] = byte_3B85E[index];
	}
	if (track_setup() != 0)
		return 1;

	cvxptr = mmgr_alloc_resbytes("cvx", 0x5780);
	init_game_state(-1);
	word_449EA = -1;
	run_game_random = LEGACY_S16_SHL(get_kevinrandom(), 3U);
	replaybar_toggle = 0;
	is_in_replay = 1;
	idle_expired = 0;
	cameramode = (legacy_s8)(camera_number - 1);
	game_replay_mode = 2;
	detail_level = 0;
	slow_video_mgmt = 0;
	slow_video_mgmt_copy = 0;

	if (setup_player_cars() != 0)
		return 1;

	kbormouse = 0;
	byte_449E6 = 0;
	byte_449DA = 1;
	game_replay_mode_copy = -1;
	byte_44346 = 0;
	byte_4432A = 0;
	byte_46467 = 0;
	dashb_toggle = 0;
	followOpponentFlag = (legacy_u8)target;
	framespersec = 20;
	word_44DCA = 0x1F4;
	rect_windshield.left = 0;
	rect_windshield.right = 320;
	rect_windshield.top = 0;
	rect_windshield.bottom = 200;
	set_projection(0x23, 200 / 6, 320, 200);

	restore_gamestate(0);
	restore_gamestate(gameconfig.game_recordedframes);
	if (bmp_mode != 0)
		return pixldump_write_requested_frame(output_name, requested_frame);
	return pixldump_write_frames(output_name);
}

legacy_s16 stuntsmain(legacy_s16 argc, legacy_s8* argv[])
{
	legacy_s16 length;
	legacy_s16 result;
	legacy_s16 camera_number;
	legacy_s16 target;
	legacy_s16 bmp_mode;
	legacy_u16 requested_frame;
	legacy_s8 output_name[PIXLDUMP_OUTPUT_NAME_SIZE];

	if (argc != 4 && argc != 5) {
		pixldump_write_usage();
		return 1;
	}
	camera_number = pixldump_parse_camera(argv[2]);
	if (camera_number == 0) {
		pixldump_write_stdout(
			"Camera must be 1 (F1), 2 (F2), 3 (F3), or 4 (F4).\r\n");
		return 1;
	}
	target = pixldump_parse_target(argv[3]);
	if (target < 0) {
		pixldump_write_stdout("Target must be 0 (player) or 1 (opponent).\r\n");
		return 1;
	}
	bmp_mode = argc == 5;
	requested_frame = 0;
	if (bmp_mode != 0 && !pixldump_parse_frame(argv[4], &requested_frame)) {
		pixldump_write_stdout("Frame must be an integer from 0 to 65535.\r\n");
		return 1;
	}

	length = (legacy_s16)strlen(argv[1]);
	if (length >= 4 &&
		(strcmp(argv[1] + length - 4, ".rpl") == 0 ||
		 strcmp(argv[1] + length - 4, ".RPL") == 0)) {
		argv[1][length - 4] = 0;
		length -= 4;
	}
	if (!pixldump_build_output_name(output_name, argv[1], camera_number,
		target, bmp_mode, requested_frame)) {
		pixldump_write_stdout("Output path is too long.\r\n");
		return 1;
	}

	init_main(argc, argv);
	init_div0();
	init_row_tables();
	mainresptr = file_load_resfile("main");
	fontdefptr = file_load_resource(0, "fontdef.fnt");
	fontnptr = file_load_resource(0, "fontn.fnt");
	font_set_fontdef();
	init_polyinfo();
	init_trackdata();
	init_unknown();
	init_kevinrandom("kevin");
	result = pixldump_process_replay(argv[1], output_name, camera_number,
		target, bmp_mode, requested_frame);

#ifdef RESTUNTS_ORIGINAL
	audio_stop_unk();
	audiodrv_atexit();
	kb_exit_handler();
	kb_shift_checking1();
	video_set_mode7();
#else
	call_exitlist();
#endif
	return result;
}
