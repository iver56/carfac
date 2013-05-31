// Copyright 2013, Google, Inc.
// Author: Ron Weiss <ronw@google.com>
//
// This C++ file is part of an implementation of Lyon's cochlear model:
// "Cascade of Asymmetric Resonators with Fast-Acting Compression"
// to supplement Lyon's upcoming book "Human and Machine Hearing"
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

#include <assert.h>

#include "sai.h"

SAI::SAI(const SAIParams& params) : params_(params) {
  assert(params_.window_width > params_.width &&
         "SAI window_width must be larger than width.");

  int buffer_width = params_.width +
      static_cast<int>((1 + static_cast<float>(params_.n_window_pos - 1)/2) *
                       params_.window_width);
  input_buffer_.setZero(params_.n_ch, buffer_width);
  output_buffer_.setZero(params_.n_ch, params_.width);

  window_.setLinSpaced(params_.window_width, kPi / params_.window_width, kPi)
      .sin();
}

void SAI::RunSegment(const std::vector<FloatArray>& input,
                     Float2dArray* output_frame) {
  assert(!input.empty() || input.size() <= params_.window_width &&
         "Unexpected input size.");
  assert(input[0].size() == params_.n_ch &&
         "Unexpected input frame size.");

  // Append new data to the input buffer.
  int n_shift = input.size();
  int shift_width = input_buffer_.cols() - n_shift;
  input_buffer_.topLeftCorner(params_.n_ch, shift_width).swap(
      input_buffer_.block(0, n_shift, params_.n_ch, shift_width));
  for (int i = 0; i < input.size(); ++i) {
    input_buffer_.block(0, shift_width + i, input[i].size(), 1) = input[i];
  }
  // Zero-pad the buffer if necessary.
  if (input.size() < params_.window_width) {
    int pad_width = params_.window_width - input.size();
    input_buffer_.topRightCorner(params_.n_ch, pad_width).setZero();
  }

  StabilizeSegment(input_buffer_, &output_buffer_);
  *output_frame = output_buffer_;
}

void SAI::StabilizeSegment(const Float2dArray& input_buffer,
                           Float2dArray* output_buffer) const {
  // Windows are always approximately 50% overlapped.
  float window_hop = params_.window_width / 2;
  int window_start = (input_buffer.cols() - params_.window_width) -
      (params_.n_window_pos - 1) * window_hop;
  int window_range_start = window_start - params_.future_lags - 1;
  int offset_range_start = window_start - params_.width;
  assert(offset_range_start >= 0);
  for (int i = 0; i < params_.n_ch; ++i) {
    // TODO(ronw): Rename this here and in the Matlab code since the
    // input doesn't have to contain naps.
    const FloatArray& nap_wave = input_buffer.row(i);
    // TODO(ronw): Smooth row.

    for (int w = 0; w < params_.n_window_pos; ++w) {
      int current_window_offset = w * window_hop;
      // Choose a trigger point.
      int trigger_time;
      const FloatArray& trigger_window =
          nap_wave.segment(window_range_start + current_window_offset,
                           params_.window_width);
      FPType peak_val = (trigger_window * window_).maxCoeff(&trigger_time);
      if (peak_val <= 0) {
        peak_val = window_.maxCoeff(&trigger_time);
      }
      trigger_time += current_window_offset;

      // Blend the window following the trigger into the output
      // buffer, weighted according to the the trigger strength (0.05
      // to near 1.0).
      FPType alpha = (0.025 + peak_val) / (0.5 + peak_val);
      output_buffer->row(i) *= 1 - alpha;
      output_buffer->row(i) += alpha *
          nap_wave.segment(trigger_time + offset_range_start, params_.width);
    }
  }
}