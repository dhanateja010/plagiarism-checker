#include "plagiarism_checker.hpp"
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

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

// Test case namespace
namespace testcase {
    std::string test_dir;
    std::map<std::string, std::shared_ptr<student_t>> students; 
    std::map<std::string, std::shared_ptr<professor_t>> professors; 
    std::vector<std::shared_ptr<submission_t>> pre_existing_codes;

    // Function declarations
    void get_students(void);
    void get_professors(void);
    void get_pre_existing_codes(void);
    void execute_testcase(std::string test_dir);
}

// Read students.txt and create student objects
void testcase::get_students(void) {
    std::string filename = testcase::test_dir + "/students.txt";
    std::ifstream in(filename);
    
    if (!in.is_open()) {
        std::cerr << "❌ Cannot open " << filename << std::endl;
        return;
    }
    
    std::string student_name;
    while (in >> student_name) {
        testcase::students[student_name] = 
            std::make_shared<student_t>(student_name);
    }
    in.close();
    
    std::cout << "✓ Loaded " << testcase::students.size() << " students" << std::endl;
}

// Read professors.txt and create professor objects
void testcase::get_professors(void) {
    std::string filename = testcase::test_dir + "/professors.txt";
    std::ifstream in(filename);
    
    if (!in.is_open()) {
        std::cerr << "❌ Cannot open " << filename << std::endl;
        return;
    }
    
    std::string professor_name;
    while (in >> professor_name) {
        testcase::professors[professor_name] = 
            std::make_shared<professor_t>(professor_name);
    }
    in.close();
    
    std::cout << "✓ Loaded " << testcase::professors.size() << " professors" << std::endl;
}

// Read originals.txt and create pre-existing submissions
void testcase::get_pre_existing_codes(void) {
    std::ifstream in(testcase::test_dir + "/originals.txt");
    
    if (!in.is_open()) {
        std::cerr << "❌ Cannot open originals.txt" << std::endl;
        return;
    }
    
    long id;
    std::string student_name;
    std::string professor_name;
    std::string code_file;
    
    while (in >> id >> student_name >> professor_name >> code_file) {
        std::shared_ptr<submission_t> submission = 
            std::make_shared<submission_t>();
        submission->id = id;
        submission->student = testcase::students[student_name];
        submission->professor = testcase::professors[professor_name];
        submission->codefile = testcase::test_dir + "/" + code_file + ".cpp";
        testcase::pre_existing_codes.push_back(submission);
    }
    in.close();
    
    std::cout << "✓ Loaded " << testcase::pre_existing_codes.size() << " original submissions" << std::endl;
}

// Main execution: Process all submissions
void testcase::execute_testcase(std::string __test_dir) {
    testcase::test_dir = __test_dir;
    
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   PLAGIARISM CHECKER - PHASE 2 (Multi-threaded System)    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    // Load students, professors, and original submissions
    std::cout << "📂 Loading test case from: " << __test_dir << "\n\n";
    testcase::get_students();
    testcase::get_professors();
    testcase::get_pre_existing_codes();
    
    // Create plagiarism checker with original submissions
    std::cout << "\n🚀 Starting Plagiarism Checker...\n";
    plagiarism_checker_t checker(testcase::pre_existing_codes);
    
    // Read and process submissions
    std::cout << "\n📥 Processing new submissions...\n\n";
    std::ifstream in(testcase::test_dir + "/submissions.txt");
    
    if (!in.is_open()) {
        std::cerr << "❌ Cannot open submissions.txt" << std::endl;
        return;
    }
    
    double timestamp;
    double old_timestamp = 0.0;
    long id;
    std::string student_name;
    std::string prof_name;
    std::string code_file;
    
    int submission_count = 0;
    
    while (in >> timestamp >> id >> student_name >> prof_name >> code_file) {
        submission_count++;
        
        // Simulate real-time submission (sleep between submissions)
        double sleep_time = timestamp - old_timestamp;
        if (sleep_time > 0) {
            std::cout << "⏳ Waiting " << std::fixed << std::setprecision(2) 
                      << sleep_time << " seconds...\n";
            sleep_microseconds((timestamp - old_timestamp) * 1000000);  // microseconds
        }
        
        // Create submission object
        std::shared_ptr<submission_t> submission = 
            std::make_shared<submission_t>();
        submission->id = id;
        submission->student = testcase::students[student_name];
        submission->professor = testcase::professors[prof_name];
        submission->codefile = testcase::test_dir + "/" + code_file + ".cpp";
        
        // Add to checker (triggers async processing)
        checker.add_submission(submission);
        
        old_timestamp = timestamp;
    }
    in.close();
    
    // Wait for all submissions to be processed
    std::cout << "\n⏳ Waiting for all submissions to be processed...\n\n";
#ifdef _WIN32
    Sleep(2000);
#else
    sleep(2);  // Give background thread time to process
#endif
    
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              PROCESSING COMPLETE                           ║\n";
    std::cout << "║   " << submission_count << " submissions processed                         \n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
}

// Entry point
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <test_directory>" << std::endl;
        std::cerr << "Example: " << argv[0] << " testcases/one" << std::endl;
        return 1;
    }
    
    try {
        testcase::execute_testcase(argv[1]);
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}