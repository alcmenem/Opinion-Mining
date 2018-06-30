#include "tagsmethod.h"

#include <dirent.h>
#include <iostream>
#include <fstream>
#include <math.h>

TagsMethod::TagsMethod(
    std::string cwd,  std::string pos, std::string neg,
    std::string test, std::string res) {
  working_dir = cwd;
  pos_dir = pos;
  neg_dir = neg;
  test_dir = test;
  results_dir = cwd + res + "tags_results.txt";
  
  // Reset the output file.
  std::ofstream reset_file(results_dir);
  reset_file.close();
}

TagsMethod::~TagsMethod() {

}

bool TagsMethod::Run() {
  // Step 1: Create the term maps
  std::cout << "\tCreating the term maps." << std::endl;
  if ( ParseTerms(pos_dir) == false || ParseTerms(neg_dir) == false ) {
    return false;
  }

  // Step 2: Parse the testing documents and find the result.
  std::cout << "\tParsing the testing documents." << std::endl;
  if ( ParseDocuments() == false ) {
    return false;
  }

  return true;
}

// For every document, update the term_set and the good(bad)_terms.
// After iterating through all the documents, calculate the weight
// for every term in the good(bad)_terms and then the score of each
// term in the term_set.
bool TagsMethod::ParseTerms(std::string directory) {

  // Read the documents from the directory in ascending order,
  // starting from the document corresponding to the index.
  size_t index = 0;

  while ( true ) {

    // Get the document name based on the directory,
    // index, and type of directory.
    std::string file_name = GetFile(directory, index, "train");

    // If a document was not found, break the loop,
    // else parse the file.
    if ( file_name == "" ) {
      std::cout << '\r' << "\tParsed " << index << " files from ";
      std::cout << directory << std::endl;
      break;
    }
    else {

      std::ifstream input_file(directory + file_name);
      if ( input_file.is_open() ) {

        // Store every term in the document, along with its frequency.
        std::unordered_map<std::string, size_t> frequencies;

        // Read the file, line by line.
        std::string line;
        while ( getline(input_file, line) ) {

          // Split the line based on the space character.
          // The start and end variables serve as pointers to the start
          // and end of every word. Iterate through the line, each time
          // finding the closest to the start variable, space character.
          // Extract the word and set it as the cur_word. Set the previously
          // curr_word as the last_word. Every set of words (as long as they
          // both are not equal to ""), is a new term. Save the term to the
          // frequencies map. Set the new start value as the position of the
          // character after the start. Stop on reaching the end of the line.
          std::string last_word = "", curr_word = "";
          size_t start = 0, end;
          while ( start < line.length() ) {

            // Find where the first space character is located.
            // If there are no spaces, meaning the returned value was
            // string::npos, set the end variable as the end of the line.
            end = line.find_first_of(" ", start);
            if ( end == std::string::npos ) {
              end = line.length();
            }

            // Extract the word based on the start and end values.
            // Save the previously curr_word, as the last_word.
            last_word = curr_word;
            curr_word = line.substr(start, end - start);

            if ( curr_word != "" && last_word != "" ) {

              // Create the term.
              std::string term = last_word + " " + curr_word;

              // If the term is not in the frequencies map, add it,
              // along with the document. If it already is, update
              // its frequency.
              auto found = frequencies.find(term);
              if ( found == frequencies.end() ) {
                frequencies.insert(std::make_pair(term, 1));
              }
              else {
                found->second++;
              }

            }

            start = end + 1;

          }
        }

        // Using the terms gathered in the frequencies map, update the
        // term_set and the good(bad)_terms maps. New terms are added in
        // the term_set with a weight of 0. New terms are added to the
        // good(bad)_terms, along with one entry in the documents map of
        // the TermInfo3 struct. If a term is already in the good(bad)_term,
        // just update the documents map with a new entry.
        for ( auto it_term : frequencies ) {

          // Get the term and the frequency for simplicity's sake.
          std::string term = it_term.first;
          size_t term_freq = it_term.second;

          // If this term does not exist in the term_set, add it
          // along with a neutral tag
          auto found_term = term_set.find(term);
          if ( found_term == term_set.end() ) {
            term_set.insert(std::make_pair(term, 0));
          }


          // If this term does not exist in the good(bad)_terms,
          // add it, else, just update the term's documents map.
          if ( directory == pos_dir ) {
            auto found_good = good_terms.find(term);
            if ( found_good == good_terms.end() ) {
              TermInfo3 terminfo;
              terminfo.documents.insert(std::make_pair(index, term_freq));
              good_terms.insert(std::make_pair(term, terminfo));
            }
            else {
              auto entry = std::make_pair(index, term_freq);
              found_good->second.documents.insert(entry);
            }
          }
          else {
            auto found_bad = bad_terms.find(term);
            if ( found_bad == bad_terms.end() ) {
              TermInfo3 terminfo;
              terminfo.documents.insert(std::make_pair(index, term_freq));
              bad_terms.insert(std::make_pair(term, terminfo));
            }
            else {
              auto entry = std::make_pair(index, term_freq);
              found_bad->second.documents.insert(entry);
            }
          }

        }

      }
      else {
        std::cout << "\tError: Could not open file ";
        std::cout << directory << std::endl;
        return false;
      }

      // Have a counter notifying the user about the progress.
      if ( index % 10 == 0) {
        std::cout << "\r\t" << index;
        fflush(stdout);
      }
      index++;
    }
  }

  // After parsing all positive and negative documents, calculate the weight
  // for every term in the good(bad)_terms, and then the score for each term
  // in the term_set.
  if ( directory == neg_dir ) {
    
    std::cout << "\tFinalizing the hashmaps." << std::endl;

    // Calculate the weight for every term in the good_terms.
    for ( auto it_term : good_terms ) {

      size_t weight_sum = 0;

      for ( auto it_docs : it_term.second.documents ) {
        weight_sum += it_docs.second;
      }

      if ( weight_sum > max_good_freq ) {
        max_good_freq = weight_sum;
      }

      TermInfo3 new_terminfo;
      new_terminfo.documents = it_term.second.documents;
      new_terminfo.weight = weight_sum;
      good_terms[it_term.first] = new_terminfo;
    }
  
    // Calculate the weight for every term in the bad_terms.
    for ( auto it_term : bad_terms ) {

      // Calculate the weight of this term for every document it is in
      size_t weight_sum = 0;

      for ( auto it_docs : it_term.second.documents ) {
        weight_sum += it_docs.second;
      }

      if ( weight_sum > max_bad_freq ) {
        max_bad_freq = weight_sum;
      }

      TermInfo3 new_terminfo;
      new_terminfo.documents = it_term.second.documents;
      new_terminfo.weight = weight_sum;
      bad_terms[it_term.first] = new_terminfo;
    }   

    // Calculate the tag value (score) for every term in the term_set.
    for ( auto it_term : term_set ) {

      float tag_score = 0;
      size_t good_w = 0, bad_w = 0;

      // Calculate the number of pos documents the term is found in.
      auto found_good = good_terms.find(it_term.first);
      if ( found_good != good_terms.end() ) {
        good_w = found_good->second.weight;
      }

      // Calculate the number of pos documents the term is found in.
      auto found_bad = bad_terms.find(it_term.first);
      if ( found_bad != bad_terms.end() ) {
        bad_w = found_bad->second.weight;
      }

      tag_score = (good_w / max_good_freq) - (bad_w / max_bad_freq);

      term_set[it_term.first] = tag_score;

    }

  }
  
  return true;
}

// Each document has a rating. For every document, for each term in the document,
// if this term exists in the term_set, add the term's score to the rating of the
// document. If it does not exist in the term_set, ignore it. After parsing the
// whole document, if the rating is positive, the document is positive. Else, it
// is negative.
bool TagsMethod::ParseDocuments() {

  // Read the documents from the directory in ascending order,
  // starting from the document corresponding to the index.
  size_t index = 0;

  while ( true ) {
    
    // Get the document name based on the directory,
    // index, and type of directory.
    std::string file_name = GetFile(test_dir, index, "test");

    // If a document was not found, break the loop,
    // else parse the file.
    if ( file_name == "" ) {
      std::cout << '\r' << "\tParsed " << index << " files from ";
      std::cout << test_dir << std::endl;
      break;
    }
    else {

      std::ifstream input_file(test_dir + file_name);
      if ( input_file.is_open() ) {

        // The total rating score of the document.
        float rating = 0;

        // Read the file, line by line.
        std::string line;
        while ( getline (input_file, line) ) {

          // Split the line based on the space character.
          // The start and end variables serve as pointers to the start
          // and end of every word. Iterate through the line, each time
          // finding the closest to the start variable, space character.
          // Extract the word and set it as the cur_word. Set the previously
          // curr_word as the last_word. Every set of words (as long as they
          // both are not equal to ""), is a new term. Save the term to the
          // frequencies map. Set the new start value as the position of the
          // character after the start. Stop on reaching the end of the line.
          std::string last_word = "", curr_word = "";
          size_t start = 0, end;
          while ( start < line.length() ) {

            // Find where the first space character is located.
            // If there are no spaces, meaning the returned value was
            // string::npos, set the end variable as the end of the line.
            end = line.find_first_of(" ", start);
            if ( end == std::string::npos ) {
              end = line.length();
            }

            // Extract the word based on the start and end values.
            // Save the previously curr_word, as the last_word.
            last_word = curr_word;
            curr_word = line.substr(start, end - start);

            if ( curr_word != "" && last_word != "" ) {

              // Create the term.
              std::string term = last_word + " " + curr_word;

              // If the term is included in the term_set, add its score
              // to the rating of the document.
              auto found_term = term_set.find(term);
              if ( found_term != term_set.end() ) {
                rating += found_term->second;
              }

            }

            start = end + 1;

          }
        }

        int result = 1;
        if ( rating < 0 ) {
          result = 0;
        }

        // Append the result in the output file.
        std::ofstream output_file(results_dir, std::ios_base::app);
        if ( output_file.is_open() ) {
          std::string file_num = file_name.substr(0, 5);
          output_file << file_num << " " << result << std::endl;
          output_file.close();
        }
        else {
          std::cout << "\tError: Could not open results file ";
          std::cout << results_dir << std::endl;
          return false;
        }

      }
      else {
        std::cout << "\tError: Could not open input file ";
        std::cout << test_dir + file_name << std::endl;
        return false;
      }

    }

    // Have a counter notifying the user about the progress.
    if ( index % 10 == 0) {
      std::cout << "\r\t" << index;
      fflush(stdout);
    }
    index++;
  }
  
  return true;
}

// Gets a directory and an index and returns the name of the file
// corresponding to the index. For example, if the index is 2, the
// returned file will be "2_R.txt" or "00002.txt", depending on the
// type of directory (training or testing).
std::string TagsMethod::GetFile(std::string directory,
  size_t index, std::string type) {

  std::string found_file = "";

  DIR *dirp;
  struct dirent *ent;

  // Open a directory stream pointing to the directory where the
  // documents are located.
  dirp = opendir(directory.c_str());

  // If there are no errors, a pointer to the directory is returned.
  // Else, NULL is returned.
  if ( dirp != NULL ) {

    // Get the next directory entry in the stream.
    // NULL is returned when reaching the end.
    while ( (ent = readdir(dirp)) != NULL ) {

      // Get the name of the current document.
      std::string file_name = ent->d_name;

      // Exclude the . and .. directories.
      if ( file_name != "." && file_name != ".." ) {

        // The search_name string is constructed depending on the
        // type of the directory. The type can be "train" or "test".
        // Construct the search name, check if the search name points
        // to the document's name, and return.
        std::string search_name;
        if ( type == "train" ) {
          search_name = std::to_string(index) + "_";
          if ( file_name.substr(0, search_name.length() ) == search_name ) {
            found_file = file_name;
          }
        }
        else if ( type == "test") {
          size_t digits = (std::to_string(index)).length();
          std::string zeroes = "";
          for ( size_t i = 0; i < 5 - digits; i++) {
            zeroes += "0";
          }
          search_name = zeroes + std::to_string(index) + ".txt";
          if ( file_name == search_name ) {
            found_file = file_name;
          }
        }
        else {
          std::cout << "\tError: Wrong directory type: " << type << std::endl;
        }

      }

    }

    // Close the directory stream.
    closedir (dirp);
  }
  else {
    std::cout << "\tError: Could not open directory ";
    std::cout << directory << std::endl;
  }
  
  return found_file;
}