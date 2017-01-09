/****************************************************************************
 *
 *   Copyright (c) 2016 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#include "../IEKF.hpp"

void IEKF::correctBaro(const sensor_combined_s *msg)
{
	//ROS_INFO("correct baro");
	// return if no new data
	float dt = 0;
	uint64_t timestamp = msg->timestamp + msg->baro_timestamp_relative;

	if (!_sensorBaro.ready(timestamp, dt)) {
		return;
	}

	// init origin alt
	if (!_origin.altInitialized()) {
		ROS_INFO("baro origin init alt %12.2f m", double(msg->baro_alt_meter));
		_origin.altInitialize(msg->baro_alt_meter, msg->timestamp);
		_x(X::baro_bias) = msg->baro_alt_meter - msg->baro_alt_meter;
		_x(X::asl) = msg->baro_alt_meter;
	}

	// calculate residual
	Vector<float, Y_baro::n> y;
	y(Y_baro::asl) = msg->baro_alt_meter;
	Vector<float, Y_baro::n> yh;
	yh(Y_baro::asl)	= _x(X::asl) + _x(X::baro_bias);
	Vector<float, Y_baro::n> r = y - yh;

	// save pressure altitude info
	_baroAsl = y(0);

	// define R
	Matrix<float, Y_baro::n, Y_baro::n> R;
	R(Y_baro::asl, Y_baro::asl) = 1.0f / dt;

	// define H
	Matrix<float, Y_baro::n, Xe::n> H;
	H(Y_baro::asl, Xe::asl) = 1;
	H(Y_baro::asl, Xe::baro_bias) = 1;

	// kalman correction
	Vector<float, Xe::n> dxe;
	SquareMatrix<float, Xe::n> dP;
	_sensorBaro.kalmanCorrectCond(_P, H, R, r, dxe, dP);

	if (_sensorBaro.shouldCorrect()) {
		setX(applyErrorCorrection(dxe));
		setP(_P + dP);
	}
}
