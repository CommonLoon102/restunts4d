#include <dos.h>
#include <restunts.h>
#include <memmgr.h>

typedef legacy_u16 size_t;
typedef legacy_s16 FILE;

#ifdef RESTUNTS_ORIGINAL

legacy_s16 g_errno;

FILE* fopen(const legacy_s8* path, const legacy_s8* mode)
{
	legacy_u16 segm = FP_SEG(path);
	legacy_u16 offs = FP_OFF(path);
	FILE* handle;

	g_errno = 0;

	if (mode[0] == 'w') { // Create new file for writing
		__asm {
			push ds
			mov  ah, 3Ch // Create file
			mov  cx, 0 // No attributes
			mov  ds, segm
			mov  dx, offs
			int  21h
			jnc  short create_ok
			mov  ax, 0
			mov  g_errno, 1
		create_ok:
			mov  handle, ax
			pop  ds
		}
	}
	else { // Open existing file for reading
		__asm {
			push ds
			mov  ah, 3Dh // Open file
			mov  al, 0 // Read only
			mov  ds, segm
			mov  dx, offs
			int  21h
			jnc  short open_ok
			mov  ax, 0
			mov  g_errno, 1
		open_ok:
			mov  handle, ax
			pop  ds
		}
	}

	return handle;
}

legacy_s16 fclose(FILE* file)
{
	legacy_s16 res;

	__asm {
		mov  ah, 3Eh // Close file
		mov  bx, file
		int  21h
		jnc  short close_ok
		mov  ax, 0
		mov  g_errno, 1
	close_ok:
		mov  res, ax
	}

	return res;
}

size_t fwrite(const void far* src, size_t size, size_t nmemb, FILE* file)
{
	legacy_u16 segm = FP_SEG(src);
	legacy_u16 offs = FP_OFF(src);

	size_t res;
	size *= nmemb;

	__asm {
		push ds
		mov  ah, 40h // Write to file
		mov  bx, file
		mov  ds, segm
		mov  dx, offs
		mov  cx, size
		int  21h
		jnc  short write_ok
		mov  ax, 0
		mov  g_errno, 1
	write_ok:
		mov  res, ax
		pop  ds
	}

	return res;
}

void init_row_tables(void) {
	legacy_s16 i;
	for (i = 0; i < 30; i++) {
		trackrows[i] = 30 * (29 - i);
		terrainrows[i] = 30 * i;
		trackpos[i] = (29 - i) << 10;
		trackcenterpos[i] = ((29 - i) << 10) + 0x200;
		terrainpos[i] = i << 10;
		terraincenterpos[i] = (i << 10) + 0x200;
	}

	for (i = 0; i < 30; i++) {
		trackpos2[i] = i << 10;
		trackcenterpos2[i] = (i << 10) + 0x200;
	}
}

void init_trackdata(void) {
	legacy_s8 far* trkptr;
	trkptr = mmgr_alloc_resbytes("trakdata", 0x6BF3);

	td01_track_file_cpy = trkptr;

	trkptr += 0x70a;
	td02_penalty_related = trkptr;

	trkptr += 0x70a;
	trackdata3 = trkptr;

	trkptr += 0x70a;
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

	trkptr += 0x16c;
	trackdata12 = trkptr;

	trkptr += 0x0f0;
	td13_rpl_header = trkptr;

	trkptr += 0x1a;
	td14_elem_map_main = trkptr;

	trkptr += 0x385;
	td15_terr_map_main = trkptr;

	trkptr += 0x385;
	td16_rpl_buffer = trkptr;

	trkptr += 0x2ee0;
	td17_trk_elem_ordered = trkptr;

	trkptr += 0x385;
	trackdata18 = trkptr;

	trkptr += 0x385;
	trackdata19 = trkptr;

	trkptr += 0x385;
	td20_trk_file_appnd = trkptr;

	trkptr += 0x7ac;
	td21_col_from_path = trkptr;

	trkptr += 0x385;
	td22_row_from_path = trkptr;

	trkptr += 0x385;
	trackdata23 = trkptr;

	trkptr += 0x30;
}
#else

size_t fwrite(const void far* src, size_t size, size_t nmemb, FILE* file);

#endif

#ifndef RESTUNTS_ORIGINAL
extern legacy_s16 setup_player_cars_repldump(void);
extern legacy_s8 far* polyinfoptr;
static legacy_u8 far* serialized_gamestate;
static const legacy_s8 serialized_state_chunk_name[12] = {
	'g', 'a', 'm', 'e', 's', 't', 'a', 't', 'e', 0, 0, 0
};
#endif

#ifdef RESTUNTS_HEADLESS
static void headless_status(const legacy_s8* format, ...)
{
	(void)format;
}
#define printf headless_status
#endif

// First argument is the filename without the .rpl extension.
// If there is a second argument (it can by anything, usually 1), then the filename
// can contain the .rpl extension. It is useful to call this tool via batch files,
// in that case this tool will terminate normally after done, no need to press any keys.
legacy_s16 stuntsmain(legacy_s16 argc, legacy_s8* argv[]) {
	legacy_s16 i, len;
	legacy_s8 outname[13], carid[5];
	FILE* fout;

	if (argc < 2) {
		printf("Usage: %s REPLNAME\n\n", argv[0]);
		printf("Or pass second argument to exit tool automatically on completion:\n");
		printf("Usage: %s REPLNAME 1\n\n", argv[0]);
		return 1;
	}

        len = strlen(argv[1]);
        if (len >= 4 && ((strcmp(argv[1] + len - 4, ".rpl") == 0) || strcmp(argv[1] + len - 4, ".RPL") == 0)) {
                argv[1][len - 4] = '\0';
        }

	init_main(argc, argv);
#ifndef RESTUNTS_ORIGINAL
	serialized_gamestate = (legacy_u8 far*)mmgr_alloc_resbytes(
		serialized_state_chunk_name, GAMESTATE_SERIALIZED_SIZE);
	// REPLDUMP remains in text mode, so the A000 graphics aperture is unused.
	// Make it available to the high-memory pool as the transitional C port
	// grows beyond the original executable's conventional-memory footprint.
#ifndef RESTUNTS_HEADLESS
	highpool_add_block(0xA000, 0x1000, 0);
#endif
#endif
	init_div0();
	init_row_tables();

#ifdef RESTUNTS_ORIGINAL
	/* The original oracle retains its display-resource allocation order. */
	mainresptr = file_load_resfile("main");
	fontdefptr = file_load_resource(0, "fontdef.fnt");
	fontnptr = file_load_resource(0, "fontn.fnt");
	font_set_fontdef();
	init_polyinfo();
#else
	/* Shape loading still uses this arena; the matrices and display fonts do not
	 * participate in headless replay simulation. */
#ifndef RESTUNTS_HEADLESS
	polyinfoptr = mmgr_alloc_resbytes("polyinfo", 0x28A0);
#endif
#endif

	init_trackdata();

	init_unknown();

	init_kevinrandom("kevin");

	printf("File: %s\n\n", argv[1]);
	printf("Loading replay... ");
	file_load_replay("", argv[1]);
	printf("OK\n");
	_memcpy(carid, gameconfig.game_playercarid, 4);
	carid[4] = 0;
	printf("  Track: '%s' Car: '%s'\n", gameconfig.game_trackname, carid);

	printf("Copying track... ");
	_memcpy(&gameconfigcopy, &gameconfig, sizeof(struct GAMEINFO));
	for (i = 0; i < 0x70A; i++) {
		td20_trk_file_appnd[i] = td14_elem_map_main[i];
	}
	for (i = 0; i < 0x51; i++) {
		td20_trk_file_appnd[i + 0x70A] = byte_3B80C[i];
		td20_trk_file_appnd[i + 0x75B] = byte_3B85E[i];
	}
	printf("OK\n");

	printf("Setting up track... ");
	track_setup();
	printf("OK\n");

	printf("Allocating cvx... ");
	cvxptr = mmgr_alloc_resbytes("cvx", 0x5780);
	printf("OK\n");

	printf("Initializing game state... ");
	init_game_state(0xFFFF);
	printf("OK\n");

	// Inits from run_game()...
	word_449EA = 0xFFFF;
	run_game_random = get_kevinrandom() << 3;
	replaybar_toggle = 1;
	is_in_replay = 0;
	idle_expired = 0;
	cameramode = 0;
	game_replay_mode = 2;
	is_in_replay = 1;

	printf("Setup player cars... ");
#ifdef RESTUNTS_ORIGINAL
	if (setup_player_cars() != 0) {
#else
	if (setup_player_cars_repldump() != 0) {
#endif
		printf("FAIL (out of memory)\n");
		return 1;
	}
	kbormouse = 0;
	byte_449E6 = 0;
	byte_449DA = 1;
	printf("OK\n");

	printf("Set frame callback... ");
#ifdef RESTUNTS_ORIGINAL
	set_frame_callback();
#endif
	game_replay_mode_copy = 0xFF;
	byte_44346 = 0;
	byte_4432A = 0;
	byte_46467 = 0;
	dashb_toggle = 0;
	printf("OK\n");

	printf("Restore game state... ");
	cameramode = 0;
	game_replay_mode = 2;
	word_44DCA = 0x1F4;
	framespersec = 20;

	restore_gamestate(0);
	restore_gamestate(gameconfig.game_recordedframes);
	printf("OK\n");

	strcpy(outname, argv[1]);
#ifdef RESTUNTS_ORIGINAL
	strcat(outname, ".BIN");
#else
	strcat(outname, ".BNI");
#endif
	outname[12] = 0;
	printf("Creating output file '%s'... ", outname);

	fout = fopen(outname, "w");
	if (!fout) {
		printf("FAIL\n");
		return 1;
	}
	printf("OK\n");

	#ifdef RESTUNTS_ORIGINAL
	fwrite(&gameconfig.game_recordedframes, sizeof(legacy_u16), 1, fout);
	#else
	LEGACY_WRITE_U16_LE(serialized_gamestate,
		gameconfig.game_recordedframes);
	fwrite(serialized_gamestate, 2U, 1, fout);
	#endif

	printf("Processing %d frames... ", gameconfig.game_recordedframes);

	while (gameconfig.game_recordedframes > state.game_frame) {
#ifdef RESTUNTS_ORIGINAL
		input_do_checking(1);
#endif
		update_gamestate();
	#ifdef RESTUNTS_ORIGINAL
		fwrite(&state, sizeof(struct GAMESTATE), 1, fout);
	#else
		fwrite(serialized_gamestate,
			gamestate_serialize(serialized_gamestate, &state), 1, fout);
	#endif
		//printf("Current frame %d\n", state.frame);
	}

	printf("OK\n");

	fclose(fout);

#ifdef RESTUNTS_ORIGINAL
	if (argc == 2) {
	        input_do_checking(1);
	        fatal_error("\nDone.\n");
	}
	else {
	        audio_stop_unk();
	        audiodrv_atexit();
	        kb_exit_handler();
	        kb_shift_checking1();
	        video_set_mode7();
	}
#else
	ems_shutdown();
#endif

	return 0;
}
