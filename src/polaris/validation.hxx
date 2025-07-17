#pragma once
/// @brief Consistency tests for ALR files
/// @author Torphedo
/// created Jul 17 2025
#include <string>
#include "polaris.hxx"

/// @brief Check if an ALR meets all of our expectations
/// @param msg A text buffer for user-facing messages. Even if the
/// function succeeds, there might be a message.
/// @param pol The polaris instance to validate
/// @return Whether the ALR data passed validation
bool alr_validate(std::string& msg, const polaris& pol) noexcept;

/// @brief Check if a chunk meets all of our expectations
/// @param alr A reference to the polaris instance with the relevant ALR data
/// @param chunk The chunk to validate
/// @param msg A text buffer for messages to be communicated to the user. A message might be added here even if the function succeeds.
/// @param headless Whether we're running in headless mode
/// @return Whether the chunk passed validation
bool alr_chunk_validate(const al::resource& alr, const al::resource::chunk& chunk,
                        std::string& msg, bool headless) noexcept;
