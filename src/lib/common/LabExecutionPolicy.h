// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
namespace deskflow::lab {
// Network probes use isolated credentials in Candidate builds.
// Presentation-only builds have no transport or input execution.
#ifdef DESKFLOW_CANDIDATE_BUILD
inline constexpr bool kProbeNetworkExecutionEnabled = true;
inline constexpr bool kCandidateExecutionEnabled = true;
#else
inline constexpr bool kProbeNetworkExecutionEnabled = false;
inline constexpr bool kCandidateExecutionEnabled = false;
#endif
} // namespace deskflow::lab
