#!/usr/bin/python

# glib-ginterface-gen.py: service-side interface generator
#
# Generate dbus-glib 0.x service GInterfaces from the Telepathy specification.
# The master copy of this program is in the telepathy-glib repository -
# please make any changes there.
#
# Copyright (C) 2006, 2007 Collabora Limited
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

import sys
import os.path
import xml.dom.minidom

from libglibcodegen import Signature, type_to_gtype, cmp_by_name, \
        NS_TP, dbus_gutils_wincaps_to_uscore, \
        signal_to_marshal_name, method_to_glue_marshal_name


NS_TP = "http://telepathy.freedesktop.org/wiki/DbusSpec#extensions-v0"

class Generator(object):

    def __init__(self, dom, prefix, basename, signal_marshal_prefix,
                 headers, end_headers, not_implemented_func,
                 allow_havoc):
        self.dom = dom
        self.__header = []
        self.__body = []

        assert prefix.endswith('_')
        assert not signal_marshal_prefix.endswith('_')

        # The main_prefix, sub_prefix thing is to get:
        # FOO_ -> (FOO_, _)
        # FOO_SVC_ -> (FOO_, _SVC_)
        # but
        # FOO_BAR/ -> (FOO_BAR_, _)
        # FOO_BAR/SVC_ -> (FOO_BAR_, _SVC_)

        if '/' in prefix:
            main_prefix, sub_prefix = prefix.upper().split('/', 1)
            prefix = prefix.replace('/', '_')
        else:
            main_prefix, sub_prefix = prefix.upper().split('_', 1)

        self.MAIN_PREFIX_ = main_prefix + '_'
        self._SUB_PREFIX_ = '_' + sub_prefix

        self.Prefix_ = prefix
        self.Prefix = prefix.replace('_', '')
        self.prefix_ = prefix.lower()
        self.PREFIX_ = prefix.upper()

        self.basename = basename
        self.signal_marshal_prefix = signal_marshal_prefix
        self.headers = headers
        self.end_headers = end_headers
        self.not_implemented_func = not_implemented_func
        self.allow_havoc = allow_havoc

    def h(self, s):
        self.__header.append(s)

    def b(self, s):
        self.__body.append(s)

    def do_node(self, node):
        node_name = node.getAttribute('name').replace('/', '')
        node_name_mixed = self.node_name_mixed = node_name.replace('_', '')
        node_name_lc = self.node_name_lc = node_name.lower()
        node_name_uc = self.node_name_uc = node_name.upper()

        interfaces = node.getElementsByTagName('interface')
        assert len(interfaces) == 1, interfaces
        interface = interfaces[0]
        self.iface_name = interface.getAttribute('name')

        tmp = interface.getAttribute('tp:implement-service')
        if tmp == "no":
            return

        tmp = interface.getAttribute('tp:causes-havoc')
        if tmp and not self.allow_havoc:
            raise AssertionError('%s is %s' % (self.iface_name, tmp))

        self.b('static const DBusGObjectInfo _%s%s_object_info;'
               % (self.prefix_, node_name_lc))
        self.b('')

        methods = interface.getElementsByTagName('method')
        signals = interface.getElementsByTagName('signal')
        properties = interface.getElementsByTagName('property')
        # Don't put properties in dbus-glib glue
        glue_properties = []

        self.b('struct _%s%sClass {' % (self.Prefix, node_name_mixed))
        self.b('    GTypeInterface parent_class;')
        for method in methods:
            self.b('    %s %s;' % self.get_method_impl_names(method))
        self.b('};')
        self.b('')

        if signals:
            self.b('enum {')
            for signal in signals:
                self.b('    %s,' % self.get_signal_const_entry(signal))
            self.b('    N_%s_SIGNALS' % node_name_uc)
            self.b('};')
            self.b('static guint %s_signals[N_%s_SIGNALS] = {0};'
                   % (node_name_lc, node_name_uc))
            self.b('')

        self.b('static void %s%s_base_init (gpointer klass);'
               % (self.prefix_, node_name_lc))
        self.b('')

        self.b('GType')
        self.b('%s%s_get_type (void)'
               % (self.prefix_, node_name_lc))
        self.b('{')
        self.b('  static GType type = 0;')
        self.b('')
        self.b('  if (G_UNLIKELY (type == 0))')
        self.b('    {')
        self.b('      static const GTypeInfo info = {')
        self.b('        sizeof (%s%sClass),' % (self.Prefix, node_name_mixed))
        self.b('        %s%s_base_init, /* base_init */'
               % (self.prefix_, node_name_lc))
        self.b('        NULL, /* base_finalize */')
        self.b('        NULL, /* class_init */')
        self.b('        NULL, /* class_finalize */')
        self.b('        NULL, /* class_data */')
        self.b('        0,')
        self.b('        0, /* n_preallocs */')
        self.b('        NULL /* instance_init */')
        self.b('      };')
        self.b('')
        self.b('      type = g_type_register_static (G_TYPE_INTERFACE,')
        self.b('          "%s%s", &info, 0);' % (self.Prefix, node_name_mixed))
        self.b('    }')
        self.b('')
        self.b('  return type;')
        self.b('}')
        self.b('')

        self.h('/**')
        self.h(' * %s%s:' % (self.Prefix, node_name_mixed))
        self.h(' *')
        self.h(' * Dummy typedef representing any implementation of this '
               'interface.')
        self.h(' */')
        self.h('typedef struct _%s%s %s%s;'
               % (self.Prefix, node_name_mixed, self.Prefix, node_name_mixed))
        self.h('')
        self.h('/**')
        self.h(' * %s%sClass:' % (self.Prefix, node_name_mixed))
        self.h(' *')
        self.h(' * The class of %s%s.' % (self.Prefix, node_name_mixed))
        self.h(' */')
        self.h('typedef struct _%s%sClass %s%sClass;'
               % (self.Prefix, node_name_mixed, self.Prefix, node_name_mixed))
        self.h('')
        self.h('GType %s%s_get_type (void);'
               % (self.prefix_, node_name_lc))

        gtype = self.current_gtype = \
                self.MAIN_PREFIX_ + 'TYPE' + self._SUB_PREFIX_ + node_name_uc
        classname = self.Prefix + node_name_mixed

        self.h('#define %s \\\n  (%s%s_get_type ())'
               % (gtype, self.prefix_, node_name_lc))
        self.h('#define %s%s(obj) \\\n'
               '  (G_TYPE_CHECK_INSTANCE_CAST((obj), %s, %s))'
               % (self.PREFIX_, node_name_uc, gtype, classname))
        self.h('#define %sIS%s%s(obj) \\\n'
               '  (G_TYPE_CHECK_INSTANCE_TYPE((obj), %s))'
               % (self.MAIN_PREFIX_, self._SUB_PREFIX_, node_name_uc, gtype))
        self.h('#define %s%s_GET_CLASS(obj) \\\n'
               '  (G_TYPE_INSTANCE_GET_INTERFACE((obj), %s, %sClass))'
               % (self.PREFIX_, node_name_uc, gtype, classname))
        self.h('')
        self.h('')

        base_init_code = []

        for method in methods:
            self.do_method(method)

        for signal in signals:
            base_init_code.extend(self.do_signal(signal))

        self.b('static inline void')
        self.b('%s%s_base_init_once (gpointer klass G_GNUC_UNUSED)'
               % (self.prefix_, node_name_lc))
        self.b('{')

        if properties:
            self.b('  static TpDBusPropertiesMixinPropInfo properties[%d] = {'
                   % (len(properties) + 1))

            for m in properties:
                access = m.getAttribute('access')
                assert access in ('read', 'write', 'readwrite')

                if access == 'read':
                    flags = 'TP_DBUS_PROPERTIES_MIXIN_FLAG_READ'
                elif access == 'write':
                    flags = 'TP_DBUS_PROPERTIES_MIXIN_FLAG_WRITE'
                else:
                    flags = ('TP_DBUS_PROPERTIES_MIXIN_FLAG_READ | '
                             'TP_DBUS_PROPERTIES_MIXIN_FLAG_WRITE')

                self.b('      { 0, %s, "%s", 0, NULL, NULL }, /* %s */'
                       % (flags, m.getAttribute('type'), m.getAttribute('name')))

            self.b('      { 0, 0, NULL, 0, NULL, NULL }')
            self.b('  };')
            self.b('  static TpDBusPropertiesMixinIfaceInfo interface =')
            self.b('      { 0, properties, NULL, NULL };')
            self.b('')


        self.b('  dbus_g_object_type_install_info (%s%s_get_type (),'
               % (self.prefix_, node_name_lc))
        self.b('      &_%s%s_object_info);'
               % (self.prefix_, node_name_lc))
        self.b('')

        if properties:
            self.b('  interface.dbus_interface = g_quark_from_static_string '
                   '("%s");' % self.iface_name)

            for i, m in enumerate(properties):
                self.b('  properties[%d].name = g_quark_from_static_string ("%s");'
                       % (i, m.getAttribute('name')))
                self.b('  properties[%d].type = %s;'
                           % (i, type_to_gtype(m.getAttribute('type'))[1]))

            self.b('  tp_svc_interface_set_dbus_properties_info (%s, &interface);'
                   % self.current_gtype)

            self.b('')

        for s in base_init_code:
            self.b(s)
        self.b('}')

        self.b('static void')
        self.b('%s%s_base_init (gpointer klass)'
               % (self.prefix_, node_name_lc))
        self.b('{')
        self.b('  static gboolean initialized = FALSE;')
        self.b('')
        self.b('  if (!initialized)')
        self.b('    {')
        self.b('      initialized = TRUE;')
        self.b('      %s%s_base_init_once (klass);'
               % (self.prefix_, node_name_lc))
        self.b('    }')
        # insert anything we need to do per implementation here
        self.b('}')

        self.h('')

        self.b('static const DBusGMethodInfo _%s%s_methods[] = {'
               % (self.prefix_, node_name_lc))

        method_blob, offsets = self.get_method_glue(methods)

        for method, offset in zip(methods, offsets):
            self.do_method_glue(method, offset)

        if len(methods) == 0:
            # empty arrays are a gcc extension, so put in a dummy member
            self.b("  { NULL, NULL, 0 }")

        self.b('};')
        self.b('')

        self.b('static const DBusGObjectInfo _%s%s_object_info = {'
               % (self.prefix_, node_name_lc))
        self.b('  0,')  # version
        self.b('  _%s%s_methods,' % (self.prefix_, node_name_lc))
        self.b('  %d,' % len(methods))
        self.b('"' + method_blob.replace('\0', '\\0') + '",')
        self.b('"' + self.get_signal_glue(signals).replace('\0', '\\0') + '",')
        self.b('"' +
               self.get_property_glue(glue_properties).replace('\0', '\\0') +
               '",')
        self.b('};')
        self.b('')

        self.node_name_mixed = None
        self.node_name_lc = None
        self.node_name_uc = None

    def get_method_glue(self, methods):
        info = []
        offsets = []

        for method in methods:
            offsets.append(len(''.join(info)))

            info.append(self.iface_name + '\0')
            info.append(method.getAttribute('name') + '\0')

            info.append('A\0')    # async

            counter = 0
            for arg in method.getElementsByTagName('arg'):
                out = arg.getAttribute('direction') == 'out'

                name = arg.getAttribute('name')
                if not name:
                    assert out
                    name = 'arg%u' % counter
                counter += 1

                info.append(name + '\0')

                if out:
                    info.append('O\0')
                else:
                    info.append('I\0')

                if out:
                    info.append('F\0')    # not const
                    info.append('N\0')    # not error or return
                info.append(arg.getAttribute('type') + '\0')

            info.append('\0')

        return ''.join(info) + '\0', offsets

    def do_method_glue(self, method, offset):
        lc_name = method.getAttribute('tp:name-for-bindings')
        if method.getAttribute('name') != lc_name.replace('_', ''):
            raise AssertionError('Method %s tp:name-for-bindings (%s) does '
                    'not match' % (method.getAttribute('name'), lc_name))
        lc_name = lc_name.lower()

        marshaller = method_to_glue_marshal_name(method,
                self.signal_marshal_prefix)
        wrapper = self.prefix_ + self.node_name_lc + '_' + lc_name

        self.b("  { (GCallback) %s, %s, %d }," % (wrapper, marshaller, offset))

    def get_signal_glue(self, signals):
        info = []

        for signal in signals:
            info.append(self.iface_name)
            info.append(signal.getAttribute('name'))

        return '\0'.join(info) + '\0\0'

    # the implementation can be the same
    get_property_glue = get_signal_glue

    def get_method_impl_names(self, method):
        dbus_method_name = method.getAttribute('name')

        class_member_name = method.getAttribute('tp:name-for-bindings')
        if dbus_method_name != class_member_name.replace('_', ''):
            raise AssertionError('Method %s tp:name-for-bindings (%s) does '
                    'not match' % (dbus_method_name, class_member_name))
        class_member_name = class_member_name.lower()

        stub_name = (self.prefix_ + self.node_name_lc + '_' +
                     class_member_name)
        return (stub_name + '_impl', class_member_name)

    def do_method(self, method):
        assert self.node_name_mixed is not None

        in_class = []

        # Examples refer to Thing.DoStuff (su) -> ii

        # DoStuff
        dbus_method_name = method.getAttribute('name')
        # do_stuff
        class_member_name = method.getAttribute('tp:name-for-bindings')
        if dbus_method_name != class_member_name.replace('_', ''):
            raise AssertionError('Method %s tp:name-for-bindings (%s) does '
                    'not match' % (dbus_method_name, class_member_name))
        class_member_name = class_member_name.lower()

        # void tp_svc_thing_do_stuff (TpSvcThing *, const char *, guint,
        #   DBusGMethodInvocation *);
        stub_name = (self.prefix_ + self.node_name_lc + '_' +
                     class_member_name)
        # typedef void (*tp_svc_thing_do_stuff_impl) (TpSvcThing *,
        #   const char *, guint, DBusGMethodInvocation);
        impl_name = stub_name + '_impl'
        # void tp_svc_thing_return_from_do_stuff (DBusGMethodInvocation *,
        #   gint, gint);
        ret_name = (self.prefix_ + self.node_name_lc + '_return_from_' +
                    class_member_name)

        # Gather arguments
        in_args = []
        out_args = []
        for i in method.getElementsByTagName('arg'):
            name = i.getAttribute('name')
            direction = i.getAttribute('direction') or 'in'
            dtype = i.getAttribute('type')

            assert direction in ('in', 'out')

            if name:
                name = direction + '_' + name
            elif direction == 'in':
                name = direction + str(len(in_args))
            else:
                name = direction + str(len(out_args))

            ctype, gtype, marshaller, pointer = type_to_gtype(dtype)

            if pointer:
                ctype = 'const ' + ctype

            struct = (ctype, name)

            if direction == 'in':
                in_args.append(struct)
            else:
                out_args.append(struct)

        # Implementation type declaration (in header, docs in body)
        self.b('/**')
        self.b(' * %s:' % impl_name)
        self.b(' * @self: The object implementing this interface')
        for (ctype, name) in in_args:
            self.b(' * @%s: %s (FIXME, generate documentation)'
                   % (name, ctype))
        self.b(' * @context: Used to return values or throw an error')
        self.b(' *')
        self.b(' * The signature of an implementation of the D-Bus method')
        self.b(' * %s on interface %s.' % (dbus_method_name, self.iface_name))
        self.b(' */')
        self.h('typedef void (*%s) (%s%s *self,'
          % (impl_name, self.Prefix, self.node_name_mixed))
        for (ctype, name) in in_args:
            self.h('    %s%s,' % (ctype, name))
        self.h('    DBusGMethodInvocation *context);')

        # Class member (in class definition)
        in_class.append('    %s %s;' % (impl_name, class_member_name))

        # Stub definition (in body only - it's static)
        self.b('static void')
        self.b('%s (%s%s *self,'
           % (stub_name, self.Prefix, self.node_name_mixed))
        for (ctype, name) in in_args:
            self.b('    %s%s,' % (ctype, name))
        self.b('    DBusGMethodInvocation *context)')
        self.b('{')
        self.b('  %s impl = (%s%s_GET_CLASS (self)->%s);'
          % (impl_name, self.PREFIX_, self.node_name_uc, class_member_name))
        self.b('')
        self.b('  if (impl != NULL)')
        tmp = ['self'] + [name for (ctype, name) in in_args] + ['context']
        self.b('    {')
        self.b('      (impl) (%s);' % ',\n        '.join(tmp))
        self.b('    }')
        self.b('  else')
        self.b('    {')
        if self.not_implemented_func:
            self.b('      %s (context);' % self.not_implemented_func)
        else:
            self.b('      GError e = { DBUS_GERROR, ')
            self.b('           DBUS_GERROR_UNKNOWN_METHOD,')
            self.b('           "Method not implemented" };')
            self.b('')
            self.b('      dbus_g_method_return_error (context, &e);')
        self.b('    }')
        self.b('}')
        self.b('')

        # Implementation registration (in both header and body)
        self.h('void %s%s_implement_%s (%s%sClass *klass, %s impl);'
               % (self.prefix_, self.node_name_lc, class_member_name,
                  self.Prefix, self.node_name_mixed, impl_name))

        self.b('/**')
        self.b(' * %s%s_implement_%s:'
               % (self.prefix_, self.node_name_lc, class_member_name))
        self.b(' * @klass: A class whose instances implement this interface')
        self.b(' * @impl: A callback used to implement the %s D-Bus method'
               % dbus_method_name)
        self.b(' *')
        self.b(' * Register an implementation for the %s method in the vtable'
               % dbus_method_name)
        self.b(' * of an implementation of this interface. To be called from')
        self.b(' * the interface init function.')
        self.b(' */')
        self.b('void')
        self.b('%s%s_implement_%s (%s%sClass *klass, %s impl)'
               % (self.prefix_, self.node_name_lc, class_member_name,
                  self.Prefix, self.node_name_mixed, impl_name))
        self.b('{')
        self.b('  klass->%s = impl;' % class_member_name)
        self.b('}')
        self.b('')

        # Return convenience function (static inline, in header)
        self.h('/**')
        self.h(' * %s:' % ret_name)
        self.h(' * @context: The D-Bus method invocation context')
        for (ctype, name) in out_args:
            self.h(' * @%s: %s (FIXME, generate documentation)'
                   % (name, ctype))
        self.h(' *')
        self.h(' * Return successfully by calling dbus_g_method_return().')
        self.h(' * This inline function exists only to provide type-safety.')
        self.h(' */')
        tmp = (['DBusGMethodInvocation *context'] +
               [ctype + name for (ctype, name) in out_args])
        self.h('static inline')
        self.h('/* this comment is to stop gtkdoc realising this is static */')
        self.h(('void %s (' % ret_name) + (',\n    '.join(tmp)) + ');')
        self.h('static inline void')
        self.h(('%s (' % ret_name) + (',\n    '.join(tmp)) + ')')
        self.h('{')
        tmp = ['context'] + [name for (ctype, name) in out_args]
        self.h('  dbus_g_method_return (' + ',\n      '.join(tmp) + ');')
        self.h('}')
        self.h('')

        return in_class

    def get_signal_const_entry(self, signal):
        assert self.node_name_uc is not None
        return ('SIGNAL_%s_%s'
                % (self.node_name_uc, signal.getAttribute('name')))

    def do_signal(self, signal):
        assert self.node_name_mixed is not None

        in_base_init = []

        # for signal: Thing::StuffHappened (s, u)
        # we want to emit:
        # void tp_svc_thing_emit_stuff_happened (gpointer instance,
        #    const char *arg0, guint arg1);

        dbus_name = signal.getAttribute('name')

        ugly_name = signal.getAttribute('tp:name-for-bindings')
        if dbus_name != ugly_name.replace('_', ''):
            raise AssertionError('Signal %s tp:name-for-bindings (%s) does '
                    'not match' % (dbus_name, ugly_name))

        stub_name = (self.prefix_ + self.node_name_lc + '_emit_' +
                     ugly_name.lower())

        const_name = self.get_signal_const_entry(signal)

        # Gather arguments
        args = []
        for i in signal.getElementsByTagName('arg'):
            name = i.getAttribute('name')
            dtype = i.getAttribute('type')
            tp_type = i.getAttribute('tp:type')

            if name:
                name = 'arg_' + name
            else:
                name = 'arg' + str(len(args))

            ctype, gtype, marshaller, pointer = type_to_gtype(dtype)

            if pointer:
                ctype = 'const ' + ctype

            struct = (ctype, name, gtype)
            args.append(struct)

        tmp = (['gpointer instance'] +
               [ctype + name for (ctype, name, gtype) in args])

        self.h(('void %s (' % stub_name) + (',\n    '.join(tmp)) + ');')

        # FIXME: emit docs

        self.b('/**')
        self.b(' * %s:' % stub_name)
        self.b(' * @instance: The object implementing this interface')
        for (ctype, name, gtype) in args:
            self.b(' * @%s: %s (FIXME, generate documentation)'
                   % (name, ctype))
        self.b(' *')
        self.b(' * Type-safe wrapper around g_signal_emit to emit the')
        self.b(' * %s signal on interface %s.'
               % (dbus_name, self.iface_name))
        self.b(' */')

        self.b('void')
        self.b(('%s (' % stub_name) + (',\n    '.join(tmp)) + ')')
        self.b('{')
        self.b('  g_assert (instance != NULL);')
        self.b('  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, %s));'
               % (self.current_gtype))
        tmp = (['instance', '%s_signals[%s]' % (self.node_name_lc, const_name),
                '0'] + [name for (ctype, name, gtype) in args])
        self.b('  g_signal_emit (' + ',\n      '.join(tmp) + ');')
        self.b('}')
        self.b('')

        signal_name = dbus_gutils_wincaps_to_uscore(dbus_name).replace('_',
                '-')
        in_base_init.append('  /**')
        in_base_init.append('   * %s%s::%s:'
                % (self.Prefix, self.node_name_mixed, signal_name))
        for (ctype, name, gtype) in args:
            in_base_init.append('   * @%s: %s (FIXME, generate documentation)'
                   % (name, ctype))
        in_base_init.append('   *')
        in_base_init.append('   * The %s D-Bus signal is emitted whenever '
                'this GObject signal is.' % dbus_name)
        in_base_init.append('   */')
        in_base_init.append('  %s_signals[%s] ='
                            % (self.node_name_lc, const_name))
        in_base_init.append('  g_signal_new ("%s",' % signal_name)
        in_base_init.append('      G_OBJECT_CLASS_TYPE (klass),')
        in_base_init.append('      G_SIGNAL_RUN_LAST|G_SIGNAL_DETAILED,')
        in_base_init.append('      0,')
        in_base_init.append('      NULL, NULL,')
        in_base_init.append('      %s,'
                % signal_to_marshal_name(signal, self.signal_marshal_prefix))
        in_base_init.append('      G_TYPE_NONE,')
        tmp = ['%d' % len(args)] + [gtype for (ctype, name, gtype) in args]
        in_base_init.append('      %s);' % ',\n      '.join(tmp))
        in_base_init.append('')

        return in_base_init

    def have_properties(self, nodes):
        for node in nodes:
            interface =  node.getElementsByTagName('interface')[0]
            if interface.getElementsByTagName('property'):
                return True
        return False

    def __call__(self):
        nodes = self.dom.getElementsByTagName('node')
        nodes.sort(cmp_by_name)

        self.h('#include <glib-object.h>')
        self.h('#include <dbus/dbus-glib.h>')

        if self.have_properties(nodes):
            self.h('#include <telepathy-glib/dbus-properties-mixin.h>')

        self.h('')
        self.h('G_BEGIN_DECLS')
        self.h('')

        self.b('#include "%s.h"' % self.basename)
        self.b('')
        for header in self.headers:
            self.b('#include %s' % header)
        self.b('')

        for node in nodes:
            self.do_node(node)

        self.h('')
        self.h('G_END_DECLS')

        self.b('')
        for header in self.end_headers:
            self.b('#include %s' % header)

        self.h('')
        self.b('')
        open(self.basename + '.h', 'w').write('\n'.join(self.__header))
        open(self.basename + '.c', 'w').write('\n'.join(self.__body))


def cmdline_error():
    print """\
usage:
    gen-ginterface [OPTIONS] xmlfile Prefix_
options:
    --include='<header.h>' (may be repeated)
    --include='"header.h"' (ditto)
    --include-end='"header.h"' (ditto)
        Include extra headers in the generated .c file
    --signal-marshal-prefix='prefix'
        Use the given prefix on generated signal marshallers (default is
        prefix.lower()).
    --filename='BASENAME'
        Set the basename for the output files (default is prefix.lower()
        + 'ginterfaces')
    --not-implemented-func='symbol'
        Set action when methods not implemented in the interface vtable are
        called. symbol must have signature
            void symbol (DBusGMethodInvocation *context)
        and return some sort of "not implemented" error via
            dbus_g_method_return_error (context, ...)
"""
    sys.exit(1)


if __name__ == '__main__':
    from getopt import gnu_getopt

    options, argv = gnu_getopt(sys.argv[1:], '',
                               ['filename=', 'signal-marshal-prefix=',
                                'include=', 'include-end=',
                                'allow-unstable',
                                'not-implemented-func='])

    try:
        prefix = argv[1]
    except IndexError:
        cmdline_error()

    basename = prefix.lower() + 'ginterfaces'
    signal_marshal_prefix = prefix.lower().rstrip('_')
    headers = []
    end_headers = []
    not_implemented_func = ''
    allow_havoc = False

    for option, value in options:
        if option == '--filename':
            basename = value
        elif option == '--signal-marshal-prefix':
            signal_marshal_prefix = value
        elif option == '--include':
            if value[0] not in '<"':
                value = '"%s"' % value
            headers.append(value)
        elif option == '--include-end':
            if value[0] not in '<"':
                value = '"%s"' % value
            end_headers.append(value)
        elif option == '--not-implemented-func':
            not_implemented_func = value
        elif option == '--allow-unstable':
            allow_havoc = True

    try:
        dom = xml.dom.minidom.parse(argv[0])
    except IndexError:
        cmdline_error()

    Generator(dom, prefix, basename, signal_marshal_prefix, headers,
              end_headers, not_implemented_func, allow_havoc)()
