#include "meansmethod.h"
#include "tagsmethod.h"
#include "knnmethod.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm> // ::tolower

// The directory where the source code is.
std::string cwd = "/home/alex/Documents/OpinionMining";

// The directory where the result output file will be created.
std::string result_dir = "/results/";

// The directory of the input raw positive/negative/test documents
// are located.
std::string pos_dir = "/data/train/pos/";
std::string neg_dir = "/data/train/neg/";
std::string test_dir = "/data/test/";

// The directory where the parsed documents will be stored.
// Parsing documents is essentially removing any common words,
// punctiuation, and making everything lowercase.
std::string parsed_dir = "/parsedData";
std::string parsed_pos = "/pos/";
std::string parsed_neg = "/neg/";
std::string parsed_test = "/test/";

// Delimiters used to split the lines during the parsing of
// the raw documents
std::string delimiters = " .,:;?!><-\"/()";

// Common words. If any of these words are found in the raw
// documents, while parsing them, they will be discarded.
const size_t commons_size = 28;
std::string commons[commons_size] = {
  "the", "to", "of", "and", "a", "an", "that", "in",
  "it", "with", "as", "do", "there", "they", "we",
  "she", "he", "or", "will", "one", "this", "by", "so",
  "just", "i", "for", "these", "them"
};

bool CreateWindowsDir(std::string directory) {
  #if defined(_WIN32)
    int err_code = _mkdir(directory.c_str());
    if ( err_code != 0 && errno != EEXIST ) {
      perror("");
      return false;
    }
  #endif
  return true;
}

bool CreateUnixDir(std::string directory) {
  // UNIX style permissions
  mode_t perm = 0733;
  int err_code = mkdir(directory.c_str(), perm);
  if ( err_code != 0 && errno != EEXIST ) {
    perror("");
    return false;
  }
  return true;
}

// Creates the directories needed to store the parsed documents.
bool CreateDirectories() {

  bool return_value = true;

  #if defined(_WIN32)
    return_value = CreateWindowsDir(cwd + parsed_dir);
    return_value = CreateWindowsDir(cwd + parsed_dir + parsed_pos);
    return_value = CreateWindowsDir(cwd + parsed_dir + parsed_neg);
    return_value = CreateWindowsDir(cwd + parsed_dir + parsed_test);
    return_value = CreateWindowsDir(cwd + result_dir);
  #else
    return_value = CreateUnixDir(cwd + parsed_dir);
    return_value = CreateUnixDir(cwd + parsed_dir + parsed_pos);
    return_value = CreateUnixDir(cwd + parsed_dir + parsed_neg);
    return_value = CreateUnixDir(cwd + parsed_dir + parsed_test);
    return_value = CreateUnixDir(cwd + result_dir);
  #endif

  return return_value;
}

// Gets a directory and an index and returns the name of the file
// corresponding to the index. For example, if the index is 2, the
// returned file will be "2_R.txt" or "00002.txt", depending on the
// type of directory (training or testing).
std::string GetFile(std::string directory, size_t index, std::string type) {

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
          std::cout << "Error: Wrong directory type: " << type << std::endl;
        }

      }

    }

    // Close the directory stream.
    closedir (dirp);
  }
  else {
    std::cout << "Error: Could not open directory " << directory << std::endl;
  }
  
  return found_file;
}

// Parses the documents in the given directory, removing any
// common words, punctuations, and makes at letters lowercase.
bool ParseData(std::string directory, std::string type) {

  // Depending on the directory given, create the output
  // directory where the parsed documents will be stored.
  std::string output_dir;
  if ( directory == cwd + pos_dir )
    output_dir = cwd + parsed_dir + parsed_pos;
  else if ( directory == cwd + neg_dir )
    output_dir = cwd + parsed_dir + parsed_neg;
  else if ( directory == cwd + test_dir )
    output_dir = cwd + parsed_dir + parsed_test;
  else {
    std::cout << "Error: Input directory " << directory;
    std::cout << " is not valid." << std::endl;
    return false;
  }

  // Read the documents from the directory in ascending order,
  // starting from the document corresponding to the index.
  size_t index = 0;

  while ( true ) {

    // Get the document name based on the directory,
    // index, and type of directory.
    std::string file_name = GetFile(directory, index, type);

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

        // Word vector will be storing every valid word from the input
        // document. The content of the vector will be written on the
        // output document.
        std::vector<std::string> word_vector;

        // Read the file, line by line.
        std::string line;
        while ( getline(input_file, line) ) {

          // Split the line based on delimiters.
          // The start and end variables serve as pointers to the start
          // and end of every word. Iterate through the line, each time
          // finding the closest to the start variable, delimiter character.
          // Extract the word and process it. Set the new start value as the
          // position of the character after the start. Stop on reaching the
          // end of the line.
          size_t start = 0, end;
          while ( start < line.length() ) {

            // Find where the first delimiter character is located.
            // If there are no delimiters, meaning the returned value was
            // string::npos, set the end variable as the end of the line.
            end = line.find_first_of(delimiters, start);
            if ( end == std::string::npos ) {
              end = line.length();
            }

            // Extract the word based on the start and end values.
            std::string wrd = line.substr(start, end - start);

            if ( wrd.length() > 0 ) {

              // Make the word lowercase.
              std::transform(wrd.begin(), wrd.end(), wrd.begin(), ::tolower);

              // Search if this word is a common word.
              bool is_common = false;
              for ( size_t i = 0; i < commons_size; i++ ) {
                if ( wrd == commons[i] ) {
                  is_common = true;
                }
              }

              if ( is_common == false ) {
                word_vector.push_back(wrd);
              }

            }

            start = end + 1;
          }
        }

        // Write the words from the word vector on the output
        // document, seperated by a space.
        std::ofstream output_file(output_dir + file_name);
        if ( output_file.is_open() ) {
          for ( size_t i = 0; i < word_vector.size(); i++ ) {
            output_file << word_vector.at(i);
            if ( i < word_vector.size() - 1 ) {
              output_file << " ";
            }
          }
        }
        else {
          std::cout << "Error: Could not open output file ";
          std::cout << output_dir + file_name << std::endl;
          return false;
        }

        // Close the input and output file streams.
        input_file.close();
        output_file.close();
      }
      else {
        std::cout << "Error: Could not open input file ";
        std::cout << directory + file_name << std::endl;
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

bool ParseArgs(int argc, char* argv[],
  bool &pre_parse, std::string &algorithm) {

  bool return_value = true;

  if ( argc == 1) {
    std::cout << "No arguments given, running preparse and ";
    std::cout << "all algorithms." << std::endl;
    pre_parse = true;
    algorithm = "ALL";
  }
  else if ( argc > 3 ) {
    std::cout << "Error: Invalid number of arguments given - ";
    std::cout << (argc-1) << std::endl;
    return_value = false;
  }
  else {
    for ( int i = 1; i < argc; i++ ) {
      std::string arg = argv[i];
      if ( arg == "--pre-parse" ) {
        pre_parse = true;
      }
      else if ( arg == "--no-parse" ) {
        pre_parse = false;
      }
      else if ( arg == "--means" ) {
        algorithm = "MEANS";
      }
      else if ( arg == "--tags" ) {
        algorithm = "TAGS";
      }
      else if ( arg == "--k-nearest" ) {
        algorithm = "KNN";
      }
      else if ( arg == "--all" ) {
        algorithm = "ALL";
      }
      else if ( arg == "--none" ) {
        algorithm = "NONE";
      }
      else {
        std::cout << "Error: Invalid argument " << arg << std::endl;
        return_value = false;
      }
    }
  }

  return return_value;
}

bool RunMeans(size_t step) {
  
  MeansMethod meansMethod(
      cwd, cwd + parsed_dir + parsed_pos,
      cwd + parsed_dir + parsed_neg,
      cwd + parsed_dir + parsed_test, result_dir );

  std::cout << "Step " << step << ": Running means method algorithm.";
  std::cout << std::endl;

  if ( meansMethod.Run() == false ) {
    std::cout << "Error: Aborted during means method algorithm." << std::endl;
    return false;
  }

  return true;
}

bool RunTags(size_t step) {
  
  TagsMethod tagsMethod(
      cwd, cwd + parsed_dir + parsed_pos,
      cwd + parsed_dir + parsed_neg,
      cwd + parsed_dir + parsed_test, result_dir );
  
  std::cout << "Step " << step << ": Running tags method algorithm.";
  std::cout << std::endl;

  if ( tagsMethod.Run() == false ) {
    std::cout << "Error: Aborted during tags method algorithm." << std::endl;
    return false;
  }

  return true;
}

bool RunKNearest(size_t step) {

  KNNMethod knnMethod(
      cwd, cwd + parsed_dir + parsed_pos,
      cwd + parsed_dir + parsed_neg,
      cwd + parsed_dir + parsed_test, result_dir );
  
  std::cout << "Step " << step << ": Running k-nearest neighbors ";
  std::cout << "method algorithm.";
  std::cout << std::endl;

  if ( knnMethod.Run() == false ) {
    std::cout << "Error: Aborted during knn method algorithm." << std::endl;
    return false;
  }

  return true;
}

int main(int argc, char* argv[]) {

  bool pre_parse = true;
  std::string algorithm = "ALL";
  if ( ParseArgs(argc, argv, pre_parse, algorithm) == false ) {
    std::cout << "Error: Invalid arguments given." << std::endl;
    return -1;
  }

  size_t step = 0;

  std::cout << "Step " << ++step << ": Creating directories." << std::endl;
  if ( CreateDirectories() ==  false ) {
    std::cout << "Error: Could not create the directories." << std::endl;
    return -1;
  }

  if ( pre_parse == true) {
    std::cout << "Step " << ++step << ": Parsing the data." << std::endl;
    if ( ParseData(cwd + pos_dir, "train") == false ||
        ParseData(cwd + neg_dir, "train") == false ||
        ParseData(cwd + test_dir, "test") == false) {
     std::cout << "Error: Could not parse the data." << std::endl;
     return -1;
    }
  }
  else {
    std::cout << "Preparse was set to false. Skipping." << std::endl;
  }
  
  bool return_value = true;

  if ( algorithm == "MEANS" ) {
    return_value = RunMeans(++step);
  }
  else if ( algorithm == "TAGS" ) {
    return_value = RunTags(++step);
  }
  else if ( algorithm == "KNN" ) {
    return_value = RunKNearest(++step);
  }
  else if ( algorithm == "ALL" ) {
    return_value = RunMeans(++step);
    return_value = RunTags(++step);
    return_value = RunKNearest(++step);
  }
  else if ( algorithm == "NONE" ) {
    std::cout << "Running algorithm was set to false. Skipping." << std::endl;
  }
  else {

  }

  if ( return_value == false ) {
    return -1;
  }
  
  std::cout << "Done!" << std::endl;

  return 0;
}