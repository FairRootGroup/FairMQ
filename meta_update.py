#! /usr/bin/env python3
# Copyright (C) 2021-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later


import argparse
import json
import re
from collections import OrderedDict


class Manipulator(object):
    def __init__(self, regex):
        self.findregex = re.compile(regex)

    def __str__(self):
        return self.__class__.__name__

    def load(self, filename):
        with open(filename, 'rb') as fp:
            self.data = json.load(fp, object_pairs_hook=OrderedDict)

    @staticmethod
    def _dict_entry_cmp(dict1, dict2, field1, field2=None):
        if field2 is None:
            field2 = field1
        if (field1 in dict1) and (field2 in dict2):
            return dict1[field1] == dict2[field2]
        else:
            return False

    def _handle_person_list_file(self, filename, cm_field_name, **kwargs):
        fp = open(filename, 'r', encoding='utf8')
        person_list = self.data.setdefault(cm_field_name, [])
        for line in fp:
            line = line.strip()
            m = self.findregex.match(line)
            if m is None:
                raise RuntimeError("Could not analyze line %r" % line)
            found_entry = self._find_person_entry(person_list, m.groupdict())
            entry = self.update_person_entry(found_entry, m.groupdict(), **kwargs)
            if found_entry is None:
                person_list.append(entry)

    def save(self, filename, indent=2):
        with open(filename, 'w', encoding='utf8') as fp:
            json.dump(self.data, fp, indent=indent)
            fp.write('\n')


class CodeMetaManipulator(Manipulator):
    def __init__(self):
        super().__init__(r'^(?P<familyName>[-\w\s]*[-\w]),\s*'
                         r'(?P<givenName>[-\w\s]*[-\w])\s*'
                         r'(?:<(?P<email>\S+@\S+)>)?\s*'
                         r'(\[(?P<orcid>\S+)\])?$')

    def load(self, filename='codemeta.json'):
        super().load(filename)

    @classmethod
    def _find_person_entry(cls, person_list, matchdict):
        # orcid is unique
        for entry in person_list:
            if cls._dict_entry_cmp(entry, matchdict, '@id', 'orcid'):
                return entry
        for entry in person_list:
            if cls._dict_entry_cmp(entry, matchdict, 'email'):
                return entry
            if cls._dict_entry_cmp(entry, matchdict, 'familyName') \
                    and cls._dict_entry_cmp(entry, matchdict, 'givenName'):
                return entry
        return None

    @staticmethod
    def update_person_entry(entry, matchdict, **kwargs):
        if entry is None:
            entry = OrderedDict()
            entry['@type'] = 'Person'
        for field in ('givenName', 'familyName', 'email', 'orcid'):
            val = matchdict.get(field, None)
            if val is not None:
                if field == 'orcid':
                    entry['@id'] = val
                else:
                    entry[field] = val
        return entry

    def update_authors(self):
        self._handle_person_list_file('AUTHORS', 'author')
        self._handle_person_list_file('CONTRIBUTORS', 'contributor')

    def save(self, filename='codemeta.json'):
        super().save(filename)

    def version(self, new_version):
        self.data['softwareVersion'] = new_version


class ZenodoManipulator(Manipulator):
    def __init__(self):
        super().__init__(r'^(?P<name>[-\w\s,]*[-\w])\s*'
                         r'(?:<(?P<email>\S+@\S+)>)?\s*'
                         r'(\[https://orcid\.org/(?P<orcid>\S+)\])?$')

    def load(self, filename='.zenodo.json'):
        super().load(filename)

    @classmethod
    def _find_person_entry(cls, person_list, matchdict):
        # Match on orcid first
        for entry in person_list:
            if cls._dict_entry_cmp(entry, matchdict, 'orcid'):
                return entry
        for entry in person_list:
            if cls._dict_entry_cmp(entry, matchdict, 'name'):
                return entry
        return None

    @staticmethod
    def update_person_entry(entry, matchdict, **kwargs):
        if entry is None:
            entry = OrderedDict()
            if kwargs['contributor_type']:
                entry['type'] = kwargs['contributor_type']
        for field in ('name', 'orcid'):
            val = matchdict.get(field, None)
            if val is not None:
                entry[field] = val
        return entry

    def update_authors(self):
        self._handle_person_list_file('AUTHORS', 'creators')
        self._handle_person_list_file('CONTRIBUTORS', 'contributors', contributor_type='Other')

    def save(self, filename='.zenodo.json'):
        super().save(filename, 4)

    def version(self, new_version):
        self.data['version'] = new_version


def main():
    parser = argparse.ArgumentParser(description='Update codemeta.json and .zenodo.json')
    parser.add_argument('--set-version', dest='newversion')
    args = parser.parse_args()

    for manipulator in (CodeMetaManipulator(), ZenodoManipulator()):
        try:
            manipulator.load()
        except FileNotFoundError as e:
            print('*** Skipping {}: {}'.format(manipulator, e))
            continue
        if args.newversion is not None:
            manipulator.version(args.newversion)
        manipulator.update_authors()
        manipulator.save()


if __name__ == '__main__':
    main()
