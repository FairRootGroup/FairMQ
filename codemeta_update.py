#! /usr/bin/env python3
# Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later


import argparse
import json
import re
from collections import OrderedDict


class CodeMetaManipulator(object):
    def load(self, filename='codemeta.json'):
        with open(filename, 'rb') as fp:
            self.data = json.load(fp, object_pairs_hook=OrderedDict)

    @staticmethod
    def _dict_entry_cmp(dict1, dict2, field):
        if (field in dict1) and (field in dict2):
            return dict1[field] == dict2[field]
        else:
            return False

    @classmethod
    def find_person_entry(cls, person_list, matchdict):
        for entry in person_list:
            if cls._dict_entry_cmp(entry, matchdict, 'email'):
                return entry
            if cls._dict_entry_cmp(entry, matchdict, 'familyName') \
                    and cls._dict_entry_cmp(entry, matchdict, 'givenName'):
                return entry
        return None

    @staticmethod
    def update_person_entry(entry, matchdict):
        if entry is None:
            entry = OrderedDict()
            entry['@type'] = 'Person'
        for field in ('givenName', 'familyName', 'email'):
            val = matchdict.get(field, None)
            if val is not None:
                entry[field] = val
        return entry

    def handle_person_list_file(self, filename, cm_field_name):
        fp = open(filename, 'r', encoding='utf8')
        findregex = re.compile(r'^(?P<familyName>[-\w\s]*[-\w]),\s*'
                               r'(?P<givenName>[-\w\s]*[-\w])\s*'
                               r'(?:<(?P<email>\S+@\S+)>)?$')
        person_list = self.data.setdefault(cm_field_name, [])
        for line in fp:
            line = line.strip()
            m = findregex.match(line)
            if m is None:
                raise RuntimeError("Could not analyze line %r" % line)
            found_entry = self.find_person_entry(person_list, m.groupdict())
            entry = self.update_person_entry(found_entry, m.groupdict())
            if found_entry is None:
                person_list.append(entry)

    def save(self, filename='codemeta.json'):
        with open('codemeta.json', 'w', encoding='utf8') as fp:
            json.dump(self.data, fp, indent=2)
            fp.write('\n')


def main():
    parser = argparse.ArgumentParser(description='Update codemeta.json')
    parser.add_argument('--set-version', dest='newversion')
    args = parser.parse_args()

    cm = CodeMetaManipulator()
    cm.load()
    if args.newversion is not None:
        cm.data['softwareVersion'] = args.newversion
    cm.handle_person_list_file('AUTHORS', 'author')
    cm.handle_person_list_file('CONTRIBUTORS', 'contributor')
    cm.save()


if __name__ == '__main__':
    main()
