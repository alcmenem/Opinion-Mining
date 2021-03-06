#include "knnmethod.h"

#include <dirent.h>
#include <iostream>
#include <fstream>
#include <math.h>

KNNMethod::KNNMethod(
    std::string cwd,  std::string pos, std::string neg,
    std::string test, std::string res) {
  working_dir = cwd;
  pos_dir = pos;
  neg_dir = neg;
  test_dir = test;
  results_dir = cwd + res + "knn_results.txt";

  // Reset the output file.
  std::ofstream reset_file(results_dir);
  reset_file.close();
}

KNNMethod::~KNNMethod() {}

bool KNNMethod::Run() {
  // Step 1: Create the term maps.
  std::cout << "\tCreating the term maps." << std::endl;
  if ( ParseTerms(pos_dir) == false || ParseTerms(neg_dir) == false ) {
    return false;
  }

  // Step 2: Create the pos and neg vectors.
  std::cout << "\tCreating the vectors." << std::endl;
  if ( CreateVectors() == false ) {
    return false;
  }

  // Step 3: Parse the testing documents and find the result.
  std::cout << "\tParsing the testing documents." << std::endl;
  if ( ParseDocuments() == false ) {
    return false;
  }

  return true;
}

// For every document, update the term_set and the good(bad)_terms.
// For every document, store its terms in the good(bad)_docs_terms.
// For every document, calculate its maximum frequency.
// After iterating through all the documents, calculate the nidf for
// every term in the term_set and the average weight for every term in
// the good_terms and bad_terms.
bool KNNMethod::ParseTerms(std::string directory) {

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

        // Read the file, line by line
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

        std::unordered_set<std::string> terms;

        // Using the terms gathered in the frequencies map, update the
        // term_set and the good(bad)_terms maps. New terms are added in
        // the term_set along with their order of their addition, calculated
        // by the size of the term_set. Leave the nidf unset for now.
        // New terms are added to the good(bad)_terms, along with one entry
        // in the documents map of the TermInfo struct. If a term is already
        // in the good(bad)_term, just update the documents map with a new
        // entry.
        for ( auto it_term : frequencies ) {

          // Get the term and the frequency for simplicity's sake.
          std::string term = it_term.first;
          size_t term_freq = it_term.second;

          terms.insert(term);

          // If this term does not exist in the term_set, add it
          // along with its order.
          auto found_term = term_set.find(term);
          if ( found_term == term_set.end() ) {
            TermInfo5 terminfo;
            terminfo.order = term_set.size();
            term_set.insert(std::make_pair(term, terminfo));
          }

          // If this term does not exist in the good(bad)_terms,
          // add it, else, just update the term's documents map.
          if ( directory == pos_dir ) {
            auto found_good = good_terms.find(term);
            if ( found_good == good_terms.end() ) {
              TermInfo4 terminfo;
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
              TermInfo4 terminfo;
              terminfo.documents.insert(std::make_pair(index, term_freq));
              bad_terms.insert(std::make_pair(term, terminfo));
            }
            else {
              auto entry = std::make_pair(index, term_freq);
              found_bad->second.documents.insert(entry);
            }
          }

        }

        // Calculate the maximum_frequency of the document
        size_t max_freq = 0;
        for ( auto it_term : frequencies ) {
          if ( it_term.second > max_freq )
            max_freq = it_term.second;
        }

        if ( directory == pos_dir ) {
          good_docs_freq.push_back(max_freq);
          good_docs_terms.push_back(terms);
        }
        else {
          bad_docs_freq.push_back(max_freq);
          bad_docs_terms.push_back(terms);
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

  // After parsing all positive and negative documents, calculate the nidf
  // for every term in the term_set and the average weight for every term
  // in the good_terms and bad_terms.
  if ( directory == neg_dir ) {
    
    std::cout << "\tFinalizing the hashmaps." << std::endl;

    // Calculate the nidf
    for ( auto it_term : term_set ) {

      float good_freq = 0, bad_freq = 0;

      // Calculate the number of positive documents the term is found in.
      auto found_good = good_terms.find(it_term.first);
      if ( found_good != good_terms.end() ) {
        good_freq = found_good->second.documents.size();
      }

      // Calculate the number of negative documents the term is found in.
      auto found_bad = bad_terms.find(it_term.first);
      if ( found_bad != bad_terms.end() ) {
        bad_freq = found_bad->second.documents.size();
      }

      float nidf = log(train_docs / (good_freq + bad_freq)) / log(train_docs);
      if ( !(nidf > 0 && nidf <= 1) ) {
        std::cout << "\tWarning: nidf is " << nidf << std::endl;
      }

      TermInfo5 terminfo = it_term.second;
      terminfo.nidf = nidf;
      term_set[it_term.first] = terminfo;

    }

    // Calculate the average weight for each term in the good_terms.
    for ( auto it_term : good_terms ) {

      float weight_sum = 0;
      float nidf = term_set[it_term.first].nidf;

      for ( auto it_doc : it_term.second.documents ) {
        weight_sum += it_doc.second * nidf;
      }

      TermInfo4 new_terminfo;
      new_terminfo.documents = it_term.second.documents;
      new_terminfo.weight = weight_sum / good_docs_freq.size();
      if ( new_terminfo.weight < 0 || new_terminfo.weight > 1 ) {
        std::cout << "\tError: Found invalid weight value ";
        std::cout << new_terminfo.weight << std::endl;
        return false;
      }
      good_terms[it_term.first] = new_terminfo;
    }

    // Calculate the average weight for each term in the bad_terms.
    for ( auto it_term : bad_terms ) {

      float weight_sum = 0;
      float nidf = term_set[it_term.first].nidf;

      for ( auto it_doc : it_term.second.documents ) {
        weight_sum += it_doc.second * nidf;
      }

      TermInfo4 new_terminfo;
      new_terminfo.documents = it_term.second.documents;
      new_terminfo.weight = weight_sum / bad_docs_freq.size();
      if ( new_terminfo.weight < 0 || new_terminfo.weight > 1 ) {
        std::cout << "\tError: Found invalid weight value ";
        std::cout << new_terminfo.weight << std::endl;
        return false;
      }
      bad_terms[it_term.first] = new_terminfo;
    } 

  }   
  
  return true;
}

// For every term in the term_set, add the weight of this term from
// the good(bad)_terms, in the cell of the good(bad)_vector indicated
// by the term's order. If the term is not in the good(bad)_terms, 0 is
// added as the weight.
bool KNNMethod::CreateVectors() {

  good_vector.resize(term_set.size(), 0);
  bad_vector.resize(term_set.size(), 0);

  for ( auto it_term : term_set ) {

    std::string term = it_term.first;
    size_t id = it_term.second.order;

    auto found_good = good_terms.find(term);
    if ( found_good != good_terms.end() ) {
      good_vector.at(id) = found_good->second.weight;
    }

    auto found_bad = bad_terms.find(term);
    if ( found_bad != bad_terms.end() ) {
      bad_vector.at(id) = found_bad->second.weight;
    }

  }

  return true;
}

// For every document in the test directory, gather the terms in a map
// like in the ParseTerms() function. Use each term in this map, to create
// a rating_vector, that is going to be used to calculate the cosine
// similarity between this, the good_vector and the bad_vactor.
bool KNNMethod::ParseDocuments() {

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

        // Store every term in the document, along with its frequency.
        std::unordered_map<std::string, size_t> frequencies;

        // Read the file, line by line
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

        // Calculate the maximum frequency of the document.
        float max_freq = 0;
        for ( auto it_term : frequencies ) {
          if ( it_term.second > max_freq ) {
            max_freq = it_term.second;
          }
        }
        if ( max_freq == 0 ) {
          std::cout << "Warning: Maximum frequency is 0. It shoudln't be 0.";
          std::cout << std::endl;
        }

        std::unordered_map<std::string, float> test_weights;

        // For every term in frequencies that is included in the term_set,
        // calculate the weight, and add it to the correct cell of the vector.
        for ( auto it_term : frequencies ) {

          std::string term = it_term.first;
          size_t freq = it_term.second;

          float ntf = freq / max_freq;

          auto found_term = term_set.find(term);
          if ( found_term != term_set.end() ) {
            float weight = ntf * found_term->second.nidf;
            if ( weight <= 0 || weight > 1) {
              std::cout << "\tError: Found invalid weight value ";
              std::cout << weight << std::endl;
              std::cout << "freq " << freq << " maxfreq " << max_freq;
              std::cout << " nidf " << found_term->second.nidf << std::endl;
              return false;
            }
            test_weights.insert(std::make_pair(term, weight));
          }

        }

        // Create a vector to store the top k similarities.
        // Vector is initialized with similarities of -1.
        std::vector<TopKInfo> top_k_docs;
        TopKInfo empty_top_k;
        empty_top_k.similarity = -999;
        empty_top_k.rating = "UNSET";
        top_k_docs.resize(knn, empty_top_k);

        // Parse all the positive documents.
        for (size_t t_index = 0; t_index < good_docs_terms.size(); t_index++) {

          // Every document has a map, with the terms, along with the
          // weights.
          std::unordered_map<std::string, float> train_weights;

          // For every term in the document, get the weight of the term,
          // and add it to the map.
          for ( auto it_term : good_docs_terms.at(t_index) ) {
            std::string term = it_term;
            auto found_good = good_terms.find(term);
            if ( found_good != good_terms.end() ) {
              auto entry = std::make_pair(term, found_good->second.weight);
              train_weights.insert(entry);
            }
            else {
              std::cout << "\tError: Could not find term " << term;
              std::cout << " in the good_terms map." << std::endl;
            }
          }

          // Calculate the similarity between the testing document
          // and the training document.
          float similarity = CosSimResult(test_weights, train_weights);

          // Iterate through the top-k vector and add the similarity
          // if it is in the top k.
          bool placed = false;
          for ( size_t i = 0; i < top_k_docs.size() && !placed; i++ ) {
            if ( similarity > top_k_docs.at(i).similarity ) {
              for ( size_t j = top_k_docs.size() - 1; j > i; j--) {
                top_k_docs.at(j) = top_k_docs.at(j - 1);
              }

              TopKInfo top_k_info;
              top_k_info.rating = "POSITIVE";
              top_k_info.similarity = similarity;
              top_k_docs.at(i) = top_k_info;
              //break;
              placed = true;
            }
          }

        }

        // Parse all the negative documents.
        for (size_t t_index = 0; t_index < bad_docs_terms.size(); t_index++) {

          // Every document has a map, with the terms, along with the
          // weights.
          std::unordered_map<std::string, float> train_weights;

          // For every term in the document, get the weight of the term,
          // and add it to the map.
          for ( auto it_term : bad_docs_terms.at(t_index) ) {
            std::string term = it_term;
            auto found_bad = bad_terms.find(term);
            if ( found_bad != bad_terms.end() ) {
              auto entry = std::make_pair(term, found_bad->second.weight);
              train_weights.insert(entry);
            }
            else {
              std::cout << "\tError: Could not find term " << term;
              std::cout << " in the bad_terms map." << std::endl;
            }
          }

          // Calculate the similarity between the testing document
          // and the training document.
          float similarity = CosSimResult(test_weights, train_weights);

          // Iterate through the top-k vector and add the similarity
          // if it is in the top k.
          bool placed = false;
          for ( size_t i = 0; i < top_k_docs.size() && !placed; i++ ) {
            if ( similarity > top_k_docs.at(i).similarity ) {
              for ( size_t j = top_k_docs.size() - 1; j > i; j-- ) {
                top_k_docs.at(j) = top_k_docs.at(j - 1);
              }

              TopKInfo top_k_info;
              top_k_info.rating = "NEGATIVE";
              top_k_info.similarity = similarity;
              top_k_docs.at(i) = top_k_info;
              //break;
              placed = true;
            }
          }
        }

        size_t pos_count = 0, neg_count = 0;

        for ( size_t i = 0; i < top_k_docs.size(); i++ ) {
          if ( top_k_docs.at(i).rating == "POSITIVE" ) {
            pos_count++;
          }
          else if ( top_k_docs.at(i).rating == "NEGATIVE" ) {
            neg_count++;
          }
          else {
            std::cout << "\tWarning: Found rating ";
            std::cout << top_k_docs.at(i).rating << std::endl;
          }
        }

        // Append the result in the output file.
        std::ofstream output_file(results_dir, std::ios_base::app);
        if ( output_file.is_open() ) {
          //std::cout << pos_count << " " << neg_count << std::endl;
          int result;
          if ( pos_count > neg_count) {
            result = 1;
          }
          else {
            result = 0;
          }
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
    std::cout << "\r\t" << index;
    fflush(stdout);
    index++;
  }

  return true;
}


float KNNMethod::CosSimResult(std::unordered_map<std::string, float> w1,
  std::unordered_map<std::string, float> w2) {

  if ( w1.size() == 0 && w2.size() != 0 ) {
    return 0;
  }
  else if ( w1.size() != 0 && w2.size() == 0 ) {
    return 0;
  }
  else if ( w1.size() == 0 && w2.size() == 0 ) {
    return 1;
  }

  float nom = 0, denom_w1 = 0, denom_w2 = 0;

  for ( auto it_term : w1 ) {
    denom_w1 += it_term.second * it_term.second;

    std::string term = it_term.first;
    auto found_w2 = w2.find(term);
    if ( found_w2 != w2.end() ) {
      nom += it_term.second * found_w2->second;
    }
  }

  for ( auto it_term : w2 ) {
    denom_w2 += it_term.second * it_term.second;
  }

  float cosine = nom / (sqrt(denom_w1) * sqrt(denom_w2));


  if ( !(cosine>=0 && cosine<=1) ) {
    std::cout << "Woah, cosine is " << cosine << std::endl;
    std::cout << "nom " << nom << " denom_w1 " << denom_w1;
    std::cout << " denom_w2 " << denom_w2 << std::endl;
    std::cout << "w1 size " << w1.size() << " s2 size " << w2.size();
    std::cout << std::endl;
  }

  return cosine;
}


// Gets a directory and an index and returns the name of the file
// corresponding to the index. For example, if the index is 2, the
// returned file will be "2_R.txt" or "00002.txt", depending on the
// type of directory (training or testing).
std::string KNNMethod::GetFile(std::string directory,
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