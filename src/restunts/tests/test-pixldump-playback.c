#include <assert.h>
#include <string.h>

#include "../pixldump/pixldump.c"

#undef memcpy
#undef strcmp

struct GAMEINFO gameconfig;
struct GAMESTATE state;
struct RECTANGLE rect_windshield;
struct LEGACY_EXECUTION_RESIDUE legacy_execution_residue;
legacy_s8 full_redraw_frames_remaining;

static legacy_u8 framebuffer[64000];
static legacy_u8 output_bytes[65535];
static legacy_u8 palette_resource[SHAPE2D_HEADER_SIZE + 768];
static unsigned output_length, render_count, present_count, close_count;
static int render_stack_enabled;

void far *dos_memory_make_pointer(legacy_u16 segment, legacy_u16 offset)
{
	assert(segment == 40960 && offset == 0);
	return framebuffer;
}

legacy_u16 dos_file_open(const legacy_s8 *path, legacy_s16 create)
{
	(void)path;
	assert(create == DOS_FILE_CREATE);
	return 5;
}

legacy_u16 dos_file_write(legacy_u16 handle, const void far *source, legacy_u16 length)
{
	assert(handle == 5);
	assert(output_length + length <= sizeof(output_bytes));
	memcpy(output_bytes + output_length, source, length);
	output_length += length;
	return length;
}

legacy_s16 dos_file_close(legacy_u16 handle)
{
	assert(handle == 5);
	close_count++;
	return 0;
}

void sprite_select_render_window(void)
{
}

void update_frame(legacy_s8 index, struct RECTANGLE *clip)
{
	assert(index == 0 && clip == &rect_windshield);
	assert(render_stack_enabled);
	assert(render_count == (unsigned)state.game_frame);
	assert(full_redraw_frames_remaining == (render_count == 0));
	/* Each frame changes one pixel using its previous contents. Skipping an
	 * intermediate render therefore changes the eventual hash and BMP too. */
	framebuffer[0] += (legacy_u8)(state.game_frame + 1);
	render_count++;
}

void frame_present(struct RECTANGLE *clip)
{
	assert(clip == &rect_windshield);
	assert(render_count == ++present_count);
}

void mouse_draw_opaque_check(void)
{
}

void update_gamestate_with_legacy_si(legacy_s16 caller_si)
{
	assert(caller_si == pixldump_caller_si);
	assert(render_count == (unsigned)state.game_frame + 1);
	state.game_frame++;
}

void shape3d_set_legacy_render_stack(legacy_s16 *headings, legacy_u16 frame_pointer,
									 legacy_u16 code_segment,
									 const struct SHAPE3D_LEGACY_OPPONENT_RENDER_CONTEXT *opponent)
{
	(void)frame_pointer;
	(void)code_segment;
	assert((headings != 0) == (opponent != 0));
	render_stack_enabled = headings != 0;
}

legacy_u16 pixldump_legacy_polygon_frame_pointer(legacy_s16 argv_si)
{
	return (legacy_u16)argv_si;
}

legacy_u16 pixldump_legacy_polygon_code_segment(void)
{
	return 0;
}

legacy_u32 pixldump_murmur3(const legacy_u8 far *source, legacy_u16 length)
{
	assert(source == framebuffer && length == sizeof(framebuffer));
	return source[0];
}

void far *file_load_shape2d_fatal(const legacy_s8 *name)
{
	assert(strcmp((const char *)name, "sdmain") == 0);
	return palette_resource;
}

legacy_s8 far *locate_shape_fatal(legacy_s8 far *data, const legacy_s8 *name)
{
	assert((void *)data == palette_resource);
	assert(strcmp((const char *)name, "!pal") == 0);
	return data;
}

void far *mmgr_free(legacy_s8 far *pointer)
{
	assert((void *)pointer == palette_resource);
	return 0;
}

static void reset_capture(void)
{
	memset(framebuffer, 0, sizeof(framebuffer));
	memset(output_bytes, 0, sizeof(output_bytes));
	state.game_frame = 0;
	full_redraw_frames_remaining = 1;
	output_length = render_count = present_count = close_count = 0;
	render_stack_enabled = 0;
}

static void test_hash_capture(void)
{
	reset_capture();
	gameconfig.game_recordedframes = 12;
	assert(pixldump_write_frames((const legacy_s8 *)"test.PDD") == 0);
	static const char expected[] = "PIXLDUMP 2\r\n"
								   "0 00000001\r\n"
								   "1 00000003\r\n"
								   "2 00000006\r\n"
								   "3 0000000a\r\n"
								   "4 0000000f\r\n"
								   "5 00000015\r\n"
								   "6 0000001c\r\n"
								   "7 00000024\r\n"
								   "8 0000002d\r\n"
								   "9 00000037\r\n"
								   "10 00000042\r\n"
								   "11 0000004e\r\n"
								   "12 0000005b\r\n";
	assert(output_length == sizeof(expected) - 1);
	assert(memcmp(output_bytes, expected, sizeof(expected) - 1) == 0);
	assert(render_count == 13 && present_count == 13);
	assert(close_count == 1 && !render_stack_enabled);
	assert(full_redraw_frames_remaining == 0);
}

static void test_bmp_capture(void)
{
	for (legacy_u16 frame = 0; frame <= 6; frame++) {
		reset_capture();
		assert(pixldump_write_requested_frame((const legacy_s8 *)"test.bmp", frame) == 0);
		assert(output_length == 65078);
		assert(output_bytes[0] == 'B' && output_bytes[1] == 'M');
		/* The BMP stores the top scanline last. */
		assert(output_bytes[1078 + 199 * 320] == (frame + 1) * (frame + 2) / 2);
		assert(render_count == (unsigned)frame + 1 && present_count == render_count);
		assert(close_count == 1 && !render_stack_enabled);
		assert(full_redraw_frames_remaining == 0);
	}
}

int main(void)
{
	test_hash_capture();
	test_bmp_capture();
	return 0;
}
