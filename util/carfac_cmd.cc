// Copyright 2013 The CARFAC Authors. All Rights Reserved.
// Author: Alex Brandmeyer
//
// This file is part of an implementation of Lyon's cochlear model:
// "Cascade of Asymmetric Resonators with Fast-Acting Compression"
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Mangled by Axel Tidemann, Boye Annfelt Hoverstad and Iver Jordal

#include <sndfile.hh>
#include "carfac.h"

#include <string>
#include <fstream>
#include <iostream>

#include <Eigen/Core>

#include "agc.h"
#include "car.h"
#include "common.h"
#include "ihc.h"

#include <chrono> // used for checking execution time of various functions

const bool DEBUG = false;

using namespace std;

// Reads a size rows by columns Eigen matrix from a text file written
// using the Matlab dlmwrite function.
ArrayXX LoadMatrix(const std::string &filename, int rows, int columns) {
    std::ifstream csv_file(filename.c_str());
    ArrayXX output(rows, columns);
    CARFAC_ASSERT(csv_file.is_open());
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            csv_file >> output(i, j);
        }
    }
    csv_file.close();
    return output;

}

// Reads sound file directly
ArrayXX LoadMatrixFromSound(const std::string &filename) {
    SndfileHandle file;

    file = SndfileHandle(filename);

    int buffer_len = file.frames() * file.channels();
    double *buffer = new double[buffer_len];

    if (DEBUG) {
        //printf("Opened file '%s'\n", filename);
        printf("Sample rate : %d\n", file.samplerate());
        printf("Channels    : %d\n", file.channels());
        printf("Frames      : %d\n", file.frames());
    }
    if (file.channels() != 1) {
        throw std::runtime_error("File must be mono (1 channel). Multi-channel sound files are not supported");
    }

    file.read(buffer, buffer_len);

    int num_columns = 1;
    ArrayXX output(buffer_len, num_columns);

    //output << buffer;

    for (int i = 0; i < buffer_len; i++) {
        output(i, 0) = buffer[i];
        //cout << buffer[i];
        //cout << endl;
    }

    delete[]buffer;

    return output;
}


void _filterloop(const ArrayX &b, const ArrayX &a, const ArrayXX &input, ArrayXX &output) {
    int order = b.rows();
    int rows = input.rows();
    int cols = input.cols();

    for (int i = 0; i < cols; i++) {
        ArrayX x = input.col(i);
        ArrayX y = output.col(i);

        for (int k = 0; k < rows; k++)
            y(k + 1) = (b * x.segment(k - order + 1, order)).sum() - (a * y.segment(k - order, order)).sum();

        output.col(i) = y;
    }
}

void filter(const ArrayX &beta, const ArrayX &alpha, const ArrayXX &input, ArrayXX &output) {

    float a0 = alpha(0);

    // For convenience with multiplication, we reverse the coefficients.
    ArrayX a = alpha.segment(1, alpha.rows() - 1).reverse() / a0;
    ArrayX b = beta.reverse() / a0;

    _filterloop(b, a, input, output);
    output = output.colwise().reverse().eval();
    _filterloop(b, a, input.colwise().reverse().eval(), output);
    output = output.colwise().reverse().eval();
}

// Seriously, I MUST LEARN how to return a pointer so I won't have to repeat code. Jesus Christ, 
// how embarrassing.
void WriteFilterMatrix(const std::string &filename, const ArrayXX &matrix, int stride, FPType a_1) {
    ArrayX b(1);
    b << 1.;
    ArrayX a(2);
    a << 1., a_1;// -0.995; //If you extend this vector, pad the matrix with N-2 zeros.

    ArrayXX filtered = ArrayXX::Zero(matrix.rows() + b.rows(), matrix.cols());
    filter(b, a, matrix, filtered);
    ArrayXX decimated(matrix.rows() / stride, matrix.cols());

    for (int i = 0; i < decimated.rows(); i++)
        decimated.row(i) = filtered.row(i * stride);

    const int kPrecision = 9;
    Eigen::IOFormat ioformat(kPrecision, Eigen::DontAlignCols);
    cout << decimated.format(ioformat) << std::endl;
}

void WriteMatrix(const std::string &filename, const ArrayXX &matrix) {
    const int kPrecision = 9;
    Eigen::IOFormat ioformat(kPrecision, Eigen::DontAlignCols);
    cout << matrix.format(ioformat) << std::endl;
}

ArrayXX LoadAudio(const std::string &filename) {
    // The Matlab audio input is transposed compared to the C++.
    return LoadMatrixFromSound(filename).transpose();
}

// Writes the CARFAC NAP output to a text file.
void WriteNAPOutput(const CARFACOutput &output, const std::string &filename,
                    int ear, int stride, FPType a_1, int apply_filter) {
    if (apply_filter)
        WriteFilterMatrix(filename, output.nap()[ear].transpose(), stride, a_1);
    else
        WriteMatrix(filename, output.nap()[ear].transpose());
}

int main(int argc, char *argv[]) {
    chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();

        CARParams car_params_;
        IHCParams ihc_params_;
        AGCParams agc_params_;
        bool open_loop_;
        std::string fname = argv[1];
        //int num_samples = atoi(argv[2]);
        //int num_ears = atoi(argv[3]);
        FPType sample_rate = 44100; //atoi(argv[4]);
        int stride = 256; // atoi(argv[5]);
        FPType a_1 = -0.995; //atof(argv[6]);
        int apply_filter = 1; //atoi(argv[7]);
        std::string suffix = ".cochlear"; //argv[8];
        ArrayXX sound_data = LoadAudio(fname);

    chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(t2 - t1).count() / 1000.0;
    if (DEBUG) {
        cout << "init and load audio: " << duration << " ms" << endl;
    }

    t1 = chrono::high_resolution_clock::now();

        int num_ears = 1;
        CARFAC carfac(num_ears, sample_rate, car_params_, ihc_params_, agc_params_);

    t2 = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::microseconds>(t2 - t1).count() / 1000.0;
    if (DEBUG) {
        cout << "init carfac: " << duration << " ms" << endl;
    }

    t1 = chrono::high_resolution_clock::now();

        CARFACOutput output(true, true, false, false);

    t2 = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::microseconds>(t2 - t1).count() / 1000.0;
    if (DEBUG) {
        cout << "carfac output: " << duration << " ms" << endl;
    }

    t1 = chrono::high_resolution_clock::now();

        carfac.RunSegment(sound_data, open_loop_, &output);

    t2 = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::microseconds>(t2 - t1).count() / 1000.0;
    if (DEBUG) {
        cout << "carfac RunSegment: " << duration << " ms" << endl;
    }

    t1 = chrono::high_resolution_clock::now();

        //If you need more ears, this is where to loop it. Change the output filename accordingly.
        WriteNAPOutput(output, fname + suffix, 0, stride, a_1, apply_filter);

    t2 = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::microseconds>(t2 - t1).count() / 1000.0;
    if (DEBUG) {
        cout << "WriteNAPOutput: " << duration << " ms" << endl;
    }
}
