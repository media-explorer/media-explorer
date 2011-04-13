#ifndef _HAVE_LIBFAKEKEY_H
#define _HAVE_LIBFAKEKEY_H

#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysymdef.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <X11/Xos.h>
#include <X11/Xproto.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup FakeKey FakeKey -
 * @brief yada yada yada
 *
 * Always remember to release held keys
 * @{
 */


/**
 * @typedef FakeKey
 *
 * The original libfakey hides this structure. The internal copy exposes it to
 * use some of the fields for the remapping.
 */
typedef struct FakeKey FakeKey;

#define N_MODIFIER_INDEXES (Mod5MapIndex + 1)

struct FakeKey
{
  Display *xdpy;
  int      min_keycode, max_keycode;
  int      n_keysyms_per_keycode;
  KeySym  *keysyms;
  int      held_keycode;
  int      held_state_flags;
  KeyCode  modifier_table[N_MODIFIER_INDEXES];
  int      shift_mod_index, alt_mod_index, meta_mod_index;
};

/**
 * @typedef FakeKeyModifier
 *
 * enumerated types for #mb_pixbuf_img_transform
 */
typedef enum
{
  FAKEKEYMOD_SHIFT   = (1<<1),
  FAKEKEYMOD_CONTROL = (1<<2),
  FAKEKEYMOD_ALT     = (1<<3),
  FAKEKEYMOD_META    = (1<<4)

} FakeKeyModifier;

/**
 * Initiates FakeKey.
 *
 * @param xdpy X Display connection.
 *
 * @return new #FakeKey reference on success, NULL on fail.
 */
FakeKey*
fakekey_init(Display *xdpy);


/**
 * Sends a Keypress to the server for the supplied UTF8 character.
 *
 * @param fk            #FakeKey refernce from #fakekey_init
 * @param utf8_char_in  Pointer to a single UTF8 Character data.
 * @param len_bytes     Lenth in bytes of character, or -1 in ends with 0
 * @param modifiers     OR'd list of #FakeKeyModifier modifiers keys
 *                      to press with the key.
 *
 *
 * @return
 */
int
fakekey_press(FakeKey             *fk,
	      const unsigned char *utf8_char_in,
	      int                  len_bytes,
	      int                  modifiers);

/**
 *  Repreats a press of the currently held key ( from #fakekey_press )
 *
 * @param fk #FakeKey refernce from #fakekey_init
 */
void
fakekey_repeat(FakeKey *fk);


/**
 * Releases the currently held key ( from #fakekey_press )
 *
 * @param fk #FakeKey refernce from #fakekey_init
 */
void
fakekey_release(FakeKey *fk);

/**
 * Resyncs the internal list of keysyms with the server.
 * Should be called if a MappingNotify event is recieved.
 *
 * @param fk #FakeKey refernce from #fakekey_init
 *
 * @return
 */
int
fakekey_reload_keysyms(FakeKey *fk);

/**
 * #fakekey_press but with an X keysym rather than a UTF8 Char.
 *
 * @param fk        #FakeKey refernce from #fakekey_init
 * @param keysym     X Keysym to send
 * @param flags
 *
 * @return
 */
int
fakekey_press_keysym(FakeKey *fk,
		     KeySym   keysym,
		     int      flags);

/**
 *
 * @param fk        #FakeKey refernce from #fakekey_init
 * @param keycode   X Keycode to send
 * @param is_press  Is this a press ( or release )
 * @param modifiers
 *
 * @return
 */
int
fakekey_send_keyevent(FakeKey *fk,
		      KeyCode  keycode,
		      Bool     is_press,
		      int      modifiers);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* _HAVE_LIBFAKEKEY_H */
