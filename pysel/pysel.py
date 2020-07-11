#!/bin/python3
from pathlib import Path
from itertools import islice, tee
from hashlib import sha1, md5
from bitarray import bitarray
import pandas as pd
import numpy as np
import pysel as sel
import sys


def read_data(filename, integer_fields):
    with open(filename, mode="r") as csv:
        workdata = pd.read_csv(csv, skipinitialspace=True)
        workdata["PLZ"] = workdata["PLZ"].astype(str)
        for f in integer_fields:
            workdata[f] = workdata[f].astype(pd.Int64Dtype())
        return workdata


def generate_ngrams(input_string, n_gram_length):
    splitter = lambda s, n: zip(*(islice(seq, index, None) for index, seq in enumerate(tee(s, n))))
    whitespace_combiner = lambda s, n: ((n-1)*' ')+s+((n-1)*' ')
    return list(splitter(whitespace_combiner(input_string, n_gram_length), n_gram_length))

def generate_bloomfilter(ngrams, bloom_length, hash_functions):
    bloom = bitarray(bloom_length)
    bloom.setall(False)
    hash_vals = []
    for ngram in ngrams:
        gram = ''.join(ngram)
        hash1 = int(sha1(gram.encode("utf-8")).hexdigest(), 16) % bloom_length
        hash2 = int(md5(gram.encode("utf-8")).hexdigest(), 16) % bloom_length
        hash_vals.append((hash1, hash2))
        for (h1, h2) in hash_vals:
            for i in range(hash_functions):
                position = (int(h1) + i*int(h2)) % bloom_length
                bloom[position] = True
    return bloom.tobytes()


def bit_to_byte(bits):
    return (bits+7)//8


class SelDatabase:
    def __init__(self, config, fieldspecs):
        self.data = read_data(config.database_file, config.integer_columns)
        self.config = config
        self.fieldspecs = fieldspecs
        self.workingmap = None

    def get_num_entries(self):
        return len(self.data)

    def get_vrecord(self):
        if self.workingmap is None:
            result = {}
            for column in self.data:
                lowercase_column = column.lower()
                if(column == "Name"): #FIXME: Dirty, dirty
                    result["nachname"] = list(self.data[column])
                elif column not in self.config.ignore_columns:
                    result[lowercase_column] = list(self.data[column])
            for k, l in result.items():
                for i, v in enumerate(l):
                    result[k][i] = to_bytes(none_if_empty(result[k][i]), self.fieldspecs[k], self.config)
            self.workingmap = result
            return result
        else:
            return self.workingmap

    def get_record_from_index(self, idx):
        return self.data.iloc[idx, :]


class SelRecords:
    def __init__(self, config, fieldspecs):
        self.data = read_data(config.record_file, config.integer_columns)
        self.config = config
        self.fieldspecs = fieldspecs
        self.workingarray = None

    def get_num_entries(self):
        return len(self.data)

    def compute_all_records(self):
            result = []
            for idx, row in self.data.iterrows():
                record = {}
                for column in row.axes[0]:
                    lowercase_column = column.lower()
                    if(column == "Name"): #FIXME: Dirty, dirty
                        record["nachname"] = to_bytes(none_if_empty(row[column]), self.fieldspecs["nachname"], self.config)
                    elif column not in self.config.ignore_columns:
                        record[lowercase_column] = to_bytes(none_if_empty(row[column]), self.fieldspecs[lowercase_column], self.config)

                result.append(record)
            self.workingarray = result
    def get_all_records(self):
        result = []
        for i in range(0, self.get_num_entries()):
            result.append(self.get_record(i))
        return result

    def get_record(self, idx):
        if self.workingarray is None:
            self.compute_all_records()
        return self.workingarray[idx]

    def get_record_from_index(self, idx):
        return self.data.iloc[idx, :]

    def get_record_from_id(self, ident):
        return self.data.set_index("Id").loc[ident]

    def set_match(self, idx, match_id, label):
        self.data[label] = 0
        self.data[label+'_id'] = 0
        self.data.iloc[idx, :][label] = 1
        self.data.iloc[idx, :][label+"_id"] = match_id


def none_if_empty(value):
    if isinstance(value, pd._libs.missing.NAType):
        return None
    else:
        if pd.isna(value):
            return None
    return value

def to_bytes(val, spec, config):
    if val is None:
        return None
    if isinstance(val, str):
        if spec.comparator == "Bitmask":
            return list(generate_bloomfilter(generate_ngrams(val, config.n_gram_length), spec.bitsize, config.hash_functions))
        if spec.comparator == "Binary":
            if config.endianness == "big":
                return list(val.encode("utf-8").rjust(bit_to_byte(spec.bitsize),b'\x00'))
            else:
                return list(val.encode("utf-8").ljust(bit_to_byte(spec.bitsize),b'\x00'))
    if isinstance(val, np.int64) or isinstance(val, int):
        if spec.comparator == "Binary":
            return list(int(val).to_bytes(length=bit_to_byte(spec.bitsize), byteorder=config.endianness))


class Configuration:
    def __init__(self, record_file, database_file):
        self.record_file = Path(record_file)
        self.database_file = Path(database_file)
        self.n_gram_length = 2
        self.bloom_length = 500
        self.hash_functions = 15
        self.bloom_columns = ['Name', 'Vorname', 'Geburtsname', 'Ort']
        self.integer_columns = ['Geburtstag', 'Geburtsmonat', 'Geburtsjahr']
        self.ignore_columns = ["Id", "Adresse"]
        self.endianness = 'big'


def main():
    """Main entry point of program"""
    if len(sys.argv) != 3:
        raise ValueError('Usage: pysel.py <recordsfile> <databasefile>')
    config = Configuration(sys.argv[1], sys.argv[2])
    epilink_config = sel.dkfz_cfg()
    database = SelDatabase(config, epilink_config.fields)
    records = SelRecords(config, epilink_config.fields)
    result = sel.v_epilink_dkfz_int(records.get_all_records(), database.get_vrecord())
    for i in result:
        print(i.index, i.match, i.tmatch, i.sum_field_weights/i.sum_weights)
    return 0

if __name__ == "__main__":
    main()
