# Tizonia's DBus-cplusplus - 'Provenance' notes

This is Tizonia's fork of https://github.com/pkgw/dbus-cplusplus, which is in
turn a fork of the original dbus-cplusplus project.

The binaries shipped in Debian/Ubuntu distros come from the original project
https://sourceforge.net/p/dbus-cplusplus. However, this project appears to have
gone un-maintained for quite some time now. Unfortunately, the original code is
not building correctly anymore with current compilers like gcc 7.2. See
https://sourceforge.net/p/dbus-cplusplus/patches/18/ for the details.

So Tizonia now embeds pkgw/dbus-cplusplus. The API has been enclosed under the
'Tiz' namespace to allow safe distribution of the main share object and helper
programs along with the other Tizonia programs and libraries.

```
namespace Tiz
{
namespace DBus
{

...

} /* namespace DBus */
} /* namespace Tiz */

```
