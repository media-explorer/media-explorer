"""Library code for language-independent D-Bus-related code generation.

The master copy of this library is in the telepathy-glib repository -
please make any changes there.
"""

# Copyright (C) 2006-2008 Collabora Limited
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


from string import ascii_letters, digits


NS_TP = "http://telepathy.freedesktop.org/wiki/DbusSpec#extensions-v0"

_ASCII_ALNUM = ascii_letters + digits


def cmp_by_name(node1, node2):
    return cmp(node1.getAttributeNode("name").nodeValue,
               node2.getAttributeNode("name").nodeValue)


def escape_as_identifier(identifier):
    """Escape the given string to be a valid D-Bus object path or service
    name component, using a reversible encoding to ensure uniqueness.

    The reversible encoding is as follows:

    * The empty string becomes '_'
    * Otherwise, each non-alphanumeric character is replaced by '_' plus
      two lower-case hex digits; the same replacement is carried out on
      the first character, if it's a digit
    """
    # '' -> '_'
    if not identifier:
        return '_'

    # A bit of a fast path for strings which are already OK.
    # We deliberately omit '_' because, for reversibility, that must also
    # be escaped.
    if (identifier.strip(_ASCII_ALNUM) == '' and
        identifier[0] in ascii_letters):
        return identifier

    # The first character may not be a digit
    if identifier[0] not in ascii_letters:
        ret = ['_%02x' % ord(identifier[0])]
    else:
        ret = [identifier[0]]

    # Subsequent characters may be digits or ASCII letters
    for c in identifier[1:]:
        if c in _ASCII_ALNUM:
            ret.append(c)
        else:
            ret.append('_%02x' % ord(c))

    return ''.join(ret)


def get_by_path(element, path):
    branches = path.split('/')
    branch = branches[0]

    # Is the current branch an attribute, if so, return the attribute value
    if branch[0] == '@':
        return element.getAttribute(branch[1:])

    # Find matching children for the branch
    children = []
    if branch == '..':
        children.append(element.parentNode)
    else:
        for x in element.childNodes:
            if x.localName == branch:
                children.append(x)

    ret = []
    # If this is not the last path element, recursively gather results from
    # children
    if len(branches) > 1:
        for x in children:
            add = get_by_path(x, '/'.join(branches[1:]))
            if isinstance(add, list):
                ret += add
            else:
                return add
    else:
        ret = children

    return ret


def get_docstring(element):
    docstring = None
    for x in element.childNodes:
        if x.namespaceURI == NS_TP and x.localName == 'docstring':
            docstring = x
    if docstring is not None:
        docstring = docstring.toxml().replace('\n', ' ').strip()
        if docstring.startswith('<tp:docstring>'):
            docstring = docstring[14:].lstrip()
        if docstring.endswith('</tp:docstring>'):
            docstring = docstring[:-15].rstrip()
        if docstring in ('<tp:docstring/>', ''):
            docstring = ''
    return docstring


def get_descendant_text(element_or_elements):
    if not element_or_elements:
        return ''
    if isinstance(element_or_elements, list):
        return ''.join(map(get_descendant_text, element_or_elements))
    parts = []
    for x in element_or_elements.childNodes:
        if x.nodeType == x.TEXT_NODE:
            parts.append(x.nodeValue)
        elif x.nodeType == x.ELEMENT_NODE:
            parts.append(get_descendant_text(x))
        else:
            pass
    return ''.join(parts)


class _SignatureIter:
    """Iterator over a D-Bus signature. Copied from dbus-python 0.71 so we
    can run genginterface in a limited environment with only Python
    (like Scratchbox).
    """
    def __init__(self, string):
        self.remaining = string

    def next(self):
        if self.remaining == '':
            raise StopIteration

        signature = self.remaining
        block_depth = 0
        block_type = None
        end = len(signature)

        for marker in range(0, end):
            cur_sig = signature[marker]

            if cur_sig == 'a':
                pass
            elif cur_sig == '{' or cur_sig == '(':
                if block_type == None:
                    block_type = cur_sig

                if block_type == cur_sig:
                    block_depth = block_depth + 1

            elif cur_sig == '}':
                if block_type == '{':
                    block_depth = block_depth - 1

                if block_depth == 0:
                    end = marker
                    break

            elif cur_sig == ')':
                if block_type == '(':
                    block_depth = block_depth - 1

                if block_depth == 0:
                    end = marker
                    break

            else:
                if block_depth == 0:
                    end = marker
                    break

        end = end + 1
        self.remaining = signature[end:]
        return Signature(signature[0:end])


class Signature(str):
    """A string, iteration over which is by D-Bus single complete types
    rather than characters.
    """
    def __iter__(self):
        return _SignatureIter(self)


def xml_escape(s):
    s = s.replace('&', '&amp;').replace("'", '&apos;').replace('"', '&quot;')
    return s.replace('<', '&lt;').replace('>', '&gt;')
