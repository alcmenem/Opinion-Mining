#ifndef TAGSMETHOD_H
#define TAGSMETHOD_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

// This struct is used by the good_terms and bad_terms unordered maps.
// Contains info about which documents the term is found, along with the
// frequency in each document. Also contains the weight of each term.
// The weight of each term is the total frequency in all the positive
// (negative) documents.
struct TermInfo3 {
  std::unordered_map<size_t, size_t> documents;
  size_t weight;
};

class TagsMethod {
public:
  TagsMethod(
        std::string cwd, std::string pos, std::string neg,
        std::string test, std::string res);
  ~TagsMethod();

  bool Run();
private:
  bool ParseTerms(std::string directory);
  bool ParseDocuments();
  std::string GetFile(std::string directory, size_t index, std::string type);

  std::string working_dir;
  std::string results_dir;
  std::string pos_dir;
  std::string neg_dir;
  std::string test_dir;

  // Stores the total unique terms from both the positive and negative
  // documents, along with a float value, indicating the score of the
  // term. Positive value means that the term is positive, negative value
  // means the term is negative. The higher the absolute value, the more the
  // weight of the term.
  std::unordered_map<std::string, float> term_set;

  // The frequency of the most common term in the positive (negative)
  // documents. Used to create normalized scores in the term_set map.
  float max_good_freq = 0, max_bad_freq = 0;

  // Stores every unique term from the positive (negative) documents,
  // along with information about the average weight of the term, and
  // the documents it is found in. For more info, refer to the TermInfo3
  // comments.
  std::unordered_map<std::string, TermInfo3> good_terms;
  std::unordered_map<std::string, TermInfo3> bad_terms;
};

#endif