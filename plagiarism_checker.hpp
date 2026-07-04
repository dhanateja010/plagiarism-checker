#ifndef PLAGIARISM_CHECKER_HPP
#define PLAGIARISM_CHECKER_HPP

#include <vector>
#include <cstddef>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <string>
#include <chrono>
#include <iostream>

// Forward declarations
class student_t;
class professor_t;

// Submission structure
struct submission_t {
    long id;                              // Unique submission ID
    std::shared_ptr<student_t> student;   // Student who submitted
    std::shared_ptr<professor_t> professor; // Professor receiving it
    std::string codefile;                 // Path to .cpp file
};

// Class for student representation
class student_t {
private:
    std::string name;

public:
    student_t(std::string __name) : name(__name) {}
    
    void flag_student(std::shared_ptr<submission_t> __submission);
    
    std::string get_name() const {
        return this->name;
    }
};

// Class for professor representation
class professor_t {
private:
    std::string name;

public:
    professor_t(std::string __name) : name(__name) {}
    
    void flag_professor(std::shared_ptr<submission_t> __submission);
    
    std::string get_name() const {
        return this->name;
    }
};

// Tokenized submission with metadata
struct tokenized_submission {
    std::shared_ptr<submission_t> submission_data;  // Original submission info
    std::vector<int> tokens;                        // Tokenized code
    std::chrono::system_clock::time_point submission_time;  // When submitted
};

// Main plagiarism checker class
class plagiarism_checker_t {
private:
    // Data members
    std::vector<tokenized_submission> original_submissions;     // Pre-existing submissions
    std::vector<tokenized_submission> checked_submissions;      // Already processed
    std::queue<tokenized_submission> submission_work_queue;     // Pending submissions
    std::thread processing_thread;                              // Background worker
    std::mutex submission_mutex;                                // Synchronization lock
    std::condition_variable cv;                                 // Thread signaling
    bool is_running;                                            // Execution flag

    // Private methods
    void evaluate_plagiarism(const tokenized_submission& current_submission);
    
    void process_pending_submissions(void);
    
    std::vector<std::pair<size_t, size_t>> find_matching_patterns(
        const std::vector<int>& current_tokens,
        const std::vector<int>& previous_tokens
    );
    
    void mark_plagiarized_submission(std::shared_ptr<submission_t> flagged_submission);

public:
    // Constructors and destructor
    plagiarism_checker_t(void);
    
    plagiarism_checker_t(std::vector<std::shared_ptr<submission_t>> __submissions);
    
    ~plagiarism_checker_t(void);
    
    // Public methods
    void add_submission(std::shared_ptr<submission_t> __submission);
};

#endif // PLAGIARISM_CHECKER_HPP