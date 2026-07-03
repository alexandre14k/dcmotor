#pragma once
#include "main.hpp"

constexpr auto TIP_PWM_REF = "Set manual PWM reference (0-100%)";
constexpr auto TIP_KP = "Proportional gain";
constexpr auto TIP_KI = "Integral gain";
constexpr auto TIP_KD = "Derivative gain";
constexpr auto TIP_KP_INC = "Increase Proportional gain. Reduces rise time but may cause overshoot.";
constexpr auto TIP_KP_DEC = "Decrease Proportional gain. Slower response, less overshoot.";
constexpr auto TIP_KI_INC = "Increase Integral gain. Eliminates steady-state error faster but increases overshoot.";
constexpr auto TIP_KI_DEC = "Decrease Integral gain. Reduces overshoot and oscillation.";
constexpr auto TIP_KD_INC = "Increase Derivative gain. Damps oscillations and improves stability.";
constexpr auto TIP_KD_DEC = "Decrease Derivative gain. Less damping, potentially more oscillation.";

constexpr auto TIP_R = "Resistance";
constexpr auto TIP_J = "Inertia";
constexpr auto TIP_B = "Friction";
constexpr auto TIP_KT = "Torque constant";
constexpr auto TIP_KE = "Back-EMF constant";
constexpr auto TIP_R_INC = "Increase Resistance. Reduces max current and torque.";
constexpr auto TIP_R_DEC = "Decrease Resistance. Increases max current and torque.";
constexpr auto TIP_J_INC = "Increase Inertia. Motor accelerates slower, heavier rotor.";
constexpr auto TIP_J_DEC = "Decrease Inertia. Motor accelerates faster, lighter rotor.";
constexpr auto TIP_B_INC = "Increase Friction. More resistance to rotation.";
constexpr auto TIP_B_DEC = "Decrease Friction. Less resistance to rotation.";
constexpr auto TIP_KT_INC = "Increase Torque constant. More torque per amp.";
constexpr auto TIP_KT_DEC = "Decrease Torque constant. Less torque per amp.";
constexpr auto TIP_KE_INC = "Increase Back-EMF constant. Motor reaches max speed at lower voltage.";
constexpr auto TIP_KE_DEC = "Decrease Back-EMF constant. Motor reaches max speed at higher voltage.";

constexpr auto TIP_STEP = "Increment/decrement step size";
constexpr auto TIP_RESET = "Reset parameter to initial value";
constexpr auto TIP_NOISE = "Toggle gaussian noise on motor";
constexpr auto TIP_NOISE_AMP = "Noise amplitude (0.001-1.0)";
constexpr auto TIP_LOG = "Toggle logarithmic plot view";
constexpr auto TIP_DISTURB_PWM = "Inject a 5% PWM pulse";
constexpr auto TIP_DISTURB_RPM = "Inject an RPM load pulse";
constexpr auto TIP_TRACK = "Enable PID to track target RPM";
constexpr auto TIP_TRACK_ENTRY = "Target RPM for tracking (0-5000)";
constexpr auto TIP_TRACK_STEP_DOWN = "Decrease target tracking RPM by 100";
constexpr auto TIP_TRACK_STEP_UP = "Increase target tracking RPM by 100";
constexpr auto TIP_PAUSE = "Pause or resume the simulation";
constexpr auto TIP_URL = "Open browser to PID controller info";
constexpr auto TIP_DEBUG = "Enable/disable CSV debug logging";
constexpr auto TIP_SPEED = "Adjust simulation and UI speed";
constexpr auto TIP_RPM_BAR = "Current motor RPM (0-5000)";
constexpr auto TIP_STATE = "Good: System is stable.\nWarning: System is close to limits (error > 100 RPM).\nSaturation: System is at limit (error > 1000 RPM).";
constexpr auto TIP_TIME_EXPAND = "Expand the time window to see more past events";
constexpr auto TIP_TIME_COMPRESS = "Compress the time window to see less past events";