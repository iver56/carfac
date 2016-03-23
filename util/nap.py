from scipy.io import wavfile
import numpy as np
from scikits.samplerate import resample
from subprocess import call
import csv
import sys


def csv_to_array(filename, delimiter=' '):
    with open(filename, 'r') as csvfile:
        reader = csv.reader(csvfile, delimiter=delimiter)
        return np.array([[float(r) for r in row] for row in reader])


def array_to_csv(filename, data, delimiter=' '):
    with open(filename, 'w') as csvfile:
        # fcntl.flock(csvfile, fcntl.LOCK_EX)
        writer = csv.writer(csvfile, delimiter=delimiter)
        if len(data.shape) == 1:
            data.shape = (data.shape[0], 1)
        for row in data:
            writer.writerow(row)
        # fcntl.flock(csvfile, fcntl.LOCK_UN)


def cochlear(filename, stride, rate, db=-40, ears=1, a_1=-0.995, apply_filter=1, suffix='cochlear'):
    original_rate, data = wavfile.read(filename)
    assert data.dtype == np.int16
    data = data / float(2 ** 15)
    if original_rate != rate:
        data = resample(data, float(rate) / original_rate, 'sinc_best')
    data = data * 10 ** (db / 20)
    array_to_csv('{}-audio.txt'.format(filename), data)
    executable_filename = './carfac-cmd'
    if sys.platform == "win32":
        executable_filename += '.exe'
    call([
        executable_filename,
        filename,
        str(len(data)),
        str(ears),
        str(rate),
        str(stride),
        str(a_1),
        str(apply_filter),
        suffix
    ])
    naps = csv_to_array(filename + suffix)
    return np.sqrt(np.maximum(0, naps) / np.max(naps))


if __name__ == '__main__':
    filename = 'drums.wav'
    stride = 256
    rate = 44100
    print cochlear(filename, stride, rate)
