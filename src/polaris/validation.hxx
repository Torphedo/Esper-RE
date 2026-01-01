#pragma once
/// @brief Consistency tests for ALR files
/// @author Torphedo
/// created Jul 17 2025
#include <string>
#include "gui/mapdata.hxx"
#include "alr/alr_file.hxx"

/// @brief Check if an ALR meets all of our expectations
/// @param msg A text buffer for user-facing messages. Even if the
/// function succeeds, there might be a message.
/// @param alr The ALR to validate
/// @param headless Whether we're running in headless mode
/// @return Whether the ALR data passed validation
bool alr_validate(std::string& msg, const alr::file& alr, bool headless) noexcept;

/// @brief Check if a chunk meets all of our expectations
/// @param alr A reference to the polaris instance with the relevant ALR data
/// @param chunk The chunk to validate
/// @param msg A text buffer for messages to be communicated to the user. A message might be added here even if the function succeeds.
/// @param headless Whether we're running in headless mode
/// @return Whether the chunk passed validation
bool alr_chunk_validate(const alr::file& alr, const alr::file::chunk& chunk,
                        std::string& msg, bool headless) noexcept;

bool mapdata_validate(const mapdata& map, std::string& msg) noexcept;
