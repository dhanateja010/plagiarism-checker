#include "plagiarism_checker.hpp"
#include "tokenizer.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace {
#ifdef _WIN32
void sleep_microseconds(double microseconds) {
    Sleep(static_cast<DWORD>(microseconds / 1000.0));
}
#else
void sleep_microseconds(double microseconds) {
    usleep(static_cast<useconds_t>(microseconds));
}
#endif
}

// ============================================================================
// STUDENT CLASS IMPLEMENTATION
// ============================================================================

void student_t::flag_student(std::shared_ptr<submission_t> __submission) {
    int i = 0;
    while (__submission->codefile[i] != '/') i++;
    i++;
    std::cout << "⚠️  I was flagged in " << __submission->codefile.substr(i) << 
        " and must defend myself in front of Prof. " <<
        __submission->professor->get_name() << "." << std::endl;
}

// ============================================================================
// PROFESSOR CLASS IMPLEMENTATION
// ============================================================================

void professor_t::flag_professor(std::shared_ptr<submission_t> __submission) {
    int i = 0;
    while (__submission->codefile[i] != '/') i++;
    i++;
    std::cout << "📋 Student " << __submission->student->get_name() << 
        " has plagiarized in " << __submission->codefile.substr(i) << 
        " and will be receiving an FR grade." << std::endl << std::endl;
}

// ============================================================================
// PLAGIARISM CHECKER - CONSTRUCTORS & DESTRUCTORS
// ============================================================================

// Default constructor: Initialize checker and start background thread
plagiarism_checker_t::plagiarism_checker_t(void) : is_running(true) {
    // Start background thread that processes submissions
    processing_thread = std::thread(&plagiarism_checker_t::process_pending_submissions, this);
    std::cout << "✓ Plagiarism Checker initialized (default)" << std::endl;
}

// Constructor with initial submissions
plagiarism_checker_t::plagiarism_checker_t(std::vector<std::shared_ptr<submission_t>> __submissions)
    : is_running(true) {
    // Start background thread
    processing_thread = std::thread(&plagiarism_checker_t::process_pending_submissions, this);

    // Tokenize and store all initial submissions
    std::cout << "⏳ Tokenizing " << __submissions.size() << " original submissions..." << std::endl;
    
    for (std::shared_ptr<submission_t> submission : __submissions) {
        try {
            // Create tokenizer for this submission's code file
            tokenizer_t tokenizer(submission->codefile);

            // Create tokenized submission structure
            tokenized_submission submission_data;
            submission_data.submission_data = submission;
            submission_data.submission_time = std::chrono::system_clock::time_point(std::chrono::seconds(0));
            submission_data.tokens = tokenizer.get_tokens();

            // Add to original submissions list
            original_submissions.push_back(submission_data);
            std::cout << "  ✓ Tokenized submission #" << submission->id << " (" 
                      << submission_data.tokens.size() << " tokens)" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "  ✗ Error tokenizing submission #" << submission->id 
                      << ": " << e.what() << std::endl;
        }
    }
    
    std::cout << "✓ Plagiarism Checker initialized with " << original_submissions.size() 
              << " original submissions" << std::endl;
}

// Destructor: Cleanup and stop background thread
plagiarism_checker_t::~plagiarism_checker_t(void) {
    {
        std::unique_lock<std::mutex> lock(submission_mutex);
        is_running = false;  // Signal thread to stop
        cv.notify_one();     // Wake up the thread
    }

    // Wait for background thread to finish
    if (processing_thread.joinable()) {
        processing_thread.join();
    }
    
    std::cout << "✓ Plagiarism Checker shut down cleanly" << std::endl;
}

// ============================================================================
// PLAGIARISM CHECKER - PUBLIC METHODS
// ============================================================================

// Add a new submission to the queue for processing
void plagiarism_checker_t::add_submission(std::shared_ptr<submission_t> __submission) {
    try {
        // Tokenize the submission's code
        tokenizer_t tokenizer(__submission->codefile);
        std::vector<int> tokens = tokenizer.get_tokens();

        // Create tokenized submission structure
        tokenized_submission submission_data;
        submission_data.submission_data = __submission;
        submission_data.submission_time = std::chrono::system_clock::now();
        submission_data.tokens = tokens;

        // Add to queue (thread-safe)
        {
            std::unique_lock<std::mutex> lock(submission_mutex);
            submission_work_queue.push(submission_data);
        }

        // Notify background thread that new work is available
        cv.notify_one();
        
        std::cout << "📥 Submission #" << __submission->id << " queued (" 
                  << tokens.size() << " tokens)" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "❌ Error processing submission #" << __submission->id 
                  << ": " << e.what() << std::endl;
    }
}

// ============================================================================
// PLAGIARISM CHECKER - PRIVATE METHODS
// ============================================================================

// Background thread: continuously process submissions from queue
void plagiarism_checker_t::process_pending_submissions(void) {
    while (is_running) {
        tokenized_submission current_submission;

        // Lock and get next submission from queue
        {
            std::unique_lock<std::mutex> lock(submission_mutex);

            // Wait until queue has submissions or we're shutting down
            while (submission_work_queue.empty() && is_running) {
                cv.wait(lock);
            }

            // If shutting down and queue is empty, exit loop
            if (!is_running && submission_work_queue.empty()) {
                break;
            }

            // Get next submission from queue
            current_submission = submission_work_queue.front();
            submission_work_queue.pop();
        }

        // Process the submission (without holding lock)
        std::cout << "⚙️  Processing submission #" << current_submission.submission_data->id << std::endl;
        evaluate_plagiarism(current_submission);
    }
}

// Evaluate if current submission contains plagiarism
void plagiarism_checker_t::evaluate_plagiarism(const tokenized_submission& current_submission) {
    auto current_time = current_submission.submission_time;
    std::vector<tokenized_submission*> submissions_to_check;

    // Add all original submissions to comparison list
    for (size_t i = 0; i < original_submissions.size(); ++i) {
        if (original_submissions[i].submission_data != current_submission.submission_data) {
            submissions_to_check.push_back(&original_submissions[i]);
        }
    }

    // Add recently checked submissions (< 1 second old)
    {
        std::unique_lock<std::mutex> lock(submission_mutex);
        for (size_t i = 0; i < checked_submissions.size(); ++i) {
            if (checked_submissions[i].submission_data != current_submission.submission_data) {
                auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(
                    current_time - checked_submissions[i].submission_time).count();
                if (time_diff <= 1) {
                    submissions_to_check.push_back(&checked_submissions[i]);
                }
            }
        }
    }

    bool plagiarism_found = false;
    std::vector<std::pair<size_t, size_t>> pattern_matches;

    // Compare against all relevant submissions
    for (auto* previous_submission : submissions_to_check) {
        auto matches = find_matching_patterns(
            current_submission.tokens,
            previous_submission->tokens
        );

        if (!matches.empty()) {
            // Check for single long match (75+ tokens)
            for (auto match : matches) {
                if (match.second >= 75) {
                    plagiarism_found = true;
                }
            }

            // Check for multiple matches (10+ patterns)
            if (matches.size() >= 10) {
                plagiarism_found = true;
            }

            // Accumulate all matching patterns
            pattern_matches.insert(pattern_matches.end(), matches.begin(), matches.end());

            // Check if submissions are very close in time (< 1 second)
            auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - previous_submission->submission_time).count();

            if (time_diff < 1) {
                std::cout << "  ⏱️  Submission #" << current_submission.submission_data->id 
                          << " submitted same time as #" << previous_submission->submission_data->id << std::endl;
                mark_plagiarized_submission(previous_submission->submission_data);
                mark_plagiarized_submission(current_submission.submission_data);
                
                // Add current to checked list and return
                {
                    std::unique_lock<std::mutex> lock(submission_mutex);
                    checked_submissions.push_back(current_submission);
                }
                return;
            }
        }
    }

    // Check for patchwork plagiarism (20+ small matching patterns)
    if (pattern_matches.size() >= 20) {
        plagiarism_found = true;
    }

    // If plagiarism detected, flag the submission
    if (plagiarism_found) {
        std::cout << "  🚨 PLAGIARISM DETECTED in submission #" 
                  << current_submission.submission_data->id << std::endl;
        mark_plagiarized_submission(current_submission.submission_data);
    } else {
        std::cout << "  ✓ No plagiarism detected in submission #" 
                  << current_submission.submission_data->id << std::endl;
    }

    // Add current submission to checked list
    {
        std::unique_lock<std::mutex> lock(submission_mutex);
        checked_submissions.push_back(current_submission);
    }
}

// Find matching patterns using KMP algorithm
std::vector<std::pair<size_t, size_t>> plagiarism_checker_t::find_matching_patterns(
    const std::vector<int>& current_tokens,
    const std::vector<int>& previous_tokens) {

    std::vector<std::pair<size_t, size_t>> matches;
    const size_t min_match_length = 15;  // Minimum tokens for a match

    // If previous tokens too short, no matches possible
    if (previous_tokens.size() < min_match_length) {
        return matches;
    }

    // Build LPS (Longest Proper Prefix Suffix) array for KMP
    std::vector<int> lps(previous_tokens.size(), 0);
    size_t len = 0;
    
    for (size_t i = 1; i < previous_tokens.size(); ) {
        if (previous_tokens[i] == previous_tokens[len]) {
            len++;
            lps[i] = len;
            i++;
        } else if (len > 0) {
            len = lps[len - 1];
        } else {
            lps[i] = 0;
            i++;
        }
    }

    // Perform KMP search to find matching patterns
    size_t i = 0, j = 0;
    while (i < current_tokens.size()) {
        if (current_tokens[i] == previous_tokens[j]) {
            i++;
            j++;

            // If we have a match long enough, record it
            if (j >= min_match_length) {
                matches.emplace_back(i - j, j);
            }

            // If we've matched the entire pattern, restart
            if (j == previous_tokens.size()) {
                j = lps[j - 1];
            }
        } else if (j > 0) {
            // Mismatch: use LPS to skip comparisons
            j = lps[j - 1];
        } else {
            // No match possible at this position
            i++;
        }
    }

    return matches;
}

// Flag a submission as plagiarized
void plagiarism_checker_t::mark_plagiarized_submission(std::shared_ptr<submission_t> flagged_submission) {
    if (flagged_submission->student != nullptr) {
        flagged_submission->student->flag_student(flagged_submission);
    }
    if (flagged_submission->professor != nullptr) {
        flagged_submission->professor->flag_professor(flagged_submission);
    }
}

// Placeholder implementation file for the plagiarism checker.
// Add your class/function definitions here.
