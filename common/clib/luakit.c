#include <gtk/gtk.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <webkit2/webkit2.h>

#include "common/clib/luakit.h"
#include "common/util.h"
#include "common/luaobject.h"

/** Get seconds from unix epoch with nanosecond precision (or nearest
 * supported by the users system).
 * \see http://www.kernel.org/doc/man-pages/online/pages/man2/clock_gettime.2.html
 *
 * \param L The Lua VM state.
 * \return  The number of elements pushed on the stack (1).
 */
gint
luaH_luakit_time(lua_State *L)
{
    lua_pushnumber(L, l_time());
    return 1;
}

/** Escapes a string for use in a URI.
 * \see http://developer.gnome.org/glib/stable/glib-URI-Functions.html#g-uri-escape-string
 *
 * \param  L The Lua VM state.
 * \return   The number of elements pushed on stack.
 *
 * \luastack
 * \lparam string  The string to escape for use in a URI.
 * \lparam allowed Optional string of allowed characters to leave unescaped in
 *                 the \c string.
 * \lreturn        The escaped string.
 */
gint
luaH_luakit_uri_encode(lua_State *L)
{
    const gchar *string = luaL_checkstring(L, 1);
    const gchar *allowed = NULL;

    /* get list of reserved characters that are allowed in the string */
    if (1 < lua_gettop(L) && !lua_isnil(L, 2))
        allowed = luaL_checkstring(L, 2);

    gchar *res = g_uri_escape_string(string, allowed, true);
    lua_pushstring(L, res);
    g_free(res);
    return 1;
}

/** Unescapes an escaped string used in a URI.
 * \see http://developer.gnome.org/glib/stable/glib-URI-Functions.html#g-uri-unescape-string
 *
 * \param  L The Lua VM state.
 * \return   The number of elements pushed on stack.
 *
 * \luastack
 * \lparam string  The string to unescape.
 * \lparam illegal Optional string of illegal chars which should not appear in
 *                 the unescaped string.
 * \lreturn        The unescaped string or \c nil if illegal chars found.
 */
gint
luaH_luakit_uri_decode(lua_State *L)
{
    const gchar *string = luaL_checkstring(L, 1);
    const gchar *illegal = NULL;

    /* get list of illegal chars not to be found in the unescaped string */
    if (1 < lua_gettop(L) && !lua_isnil(L, 2))
        illegal = luaL_checkstring(L, 2);

    gchar *res = g_uri_unescape_string(string, illegal);
    if (!res)
        return 0;

    lua_pushstring(L, res);
    g_free(res);
    return 1;
}

static lua_State *idle_cb_L;

/** Calls the idle callback function. If the callback function returns false the
 * idle source is removed, the Lua function is unreffed and will not be called
 * again.
 * \see luaH_luakit_idle_add
 *
 * \param func Lua callback function.
 * \return TRUE to keep source alive, FALSE to remove.
 */
gboolean
idle_cb(gpointer func)
{
    lua_State *L = idle_cb_L;

    /* get original stack size */
    gint top = lua_gettop(L);

    /* call function */
    luaH_object_push(L, func);
    gboolean ok = luaH_dofunction(L, 0, 1);

    /* keep the source alive? */
    gboolean keep = lua_toboolean(L, -1);

    /* allow collection of idle callback func */
    if (!keep || !ok)
        luaH_object_unref(L, func);

    /* leave stack how we found it */
    lua_settop(L, top);

    return keep && ok;
}

/** Adds a function to be called whenever there are no higher priority GTK
 * events pending in the default main loop. If the function returns false it
 * is automatically removed from the list of event sources and will not be
 * called again.
 * \see http://developer.gnome.org/glib/unstable/glib-The-Main-Event-Loop.html#g-idle-add
 *
 * \param  L The Lua VM state.
 * \return   The number of elements pushed on the stack (0).
 *
 * \luastack
 * \lparam func The callback function.
 */
gint
luaH_luakit_idle_add(lua_State *L)
{
    luaH_checkfunction(L, 1);
    idle_cb_L = L;
    gpointer func = luaH_object_ref(L, 1);
    g_idle_add(idle_cb, func);
    return 0;
}

/** Removes an idle callback by function.
 * \see http://developer.gnome.org/glib/unstable/glib-The-Main-Event-Loop.html#g-idle-remove-by-data
 *
 * \param  L The Lua VM state.
 * \return   The number of elements pushed on the stack (0).
 *
 * \luastack
 * \lparam func The callback function.
 * \lreturn true if callback removed.
 */
gint
luaH_luakit_idle_remove(lua_State *L)
{
    luaH_checkfunction(L, 1);
    gpointer func = (gpointer)lua_topointer(L, 1);
    lua_pushboolean(L, g_idle_remove_by_data(func));
    luaH_object_unref(L, func);
    return 1;
}

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80