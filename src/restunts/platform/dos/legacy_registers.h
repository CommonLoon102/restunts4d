#ifndef RESTUNTS_DOS_LEGACY_REGISTERS_H
#define RESTUNTS_DOS_LEGACY_REGISTERS_H

#if !defined(RESTUNTS_HEADLESS) && !defined(RESTUNTS_FULL)
#define LEGACY_SAVE_DX() \
	_asm { \
		push dx \
	}
#define LEGACY_RESTORE_DX() \
	_asm { \
		pop dx \
	}
#define LEGACY_SAVE_BX() \
	_asm { \
		push bx \
	}
#define LEGACY_RESTORE_BX() \
	_asm { \
		pop bx \
	}
#else
#define LEGACY_SAVE_DX() ((void)0)
#define LEGACY_RESTORE_DX() ((void)0)
#define LEGACY_SAVE_BX() ((void)0)
#define LEGACY_RESTORE_BX() ((void)0)
#endif

#endif
