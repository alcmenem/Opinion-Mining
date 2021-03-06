#ifndef KNNMETHOD_H
#define KNNMETHOD_H

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>

// Used as a value in the good_terms/bad_terms unordered maps.
// Contains the average weight of a term, calculated like this:
// (ntf(1)*nidf + ntf(2)*nidf + ... + ntf(m)*nidf) / pos(neg)_docs
// where m is the positive (negative) documents this term is found in.
// Also stores in an unordered_map the IDs of every document the term is
// found in, along with the frequency for each document.
struct TermInfo4 {
  float weight;
  std::unordered_map<size_t, size_t> documents;
};

// Used as a value in the term_set unordered map. Contains a number
// corresponding to which order this term was added to the term_set map.
// It basically works as an index used for the construction of the vectors
// later on. It also contains a number which if the nidf value for this term.
// This is an average nidf for both the pos and neg documents and the value is:
// ln((pos_docs+neg_docs) / (occurances_in_pos_docs+occurances_in_neg_docs))
// divided by ln(pos_docs+neg_docs)
struct TermInfo5 {
  size_t order;
  float nidf;
};

struct TopKInfo {
  std::string rating;
  float similarity;
};

class KNNMethod {
public:
  KNNMethod(
      std::string cwd, std::string pos, std::string neg,
      std::string test, std::string res);
  ~KNNMethod();

  bool Run();
private:
  bool ParseTerms(std::string directory);
  bool CreateVectors();
  bool ParseDocuments();
  float CosSimResult(std::unordered_map<std::string, float> w1,
      std::unordered_map<std::string, float> w2);

  std::string GetFile(std::string directory, size_t index, std::string type);

  std::string working_dir;
  std::string results_dir;
  std::string pos_dir;
  std::string neg_dir;
  std::string test_dir;

  // Stores the total unique terms from both the positive and negative
  // documents, along with information for the order added and the nidf
  // of the term. For more info, refer to the TermInfo2 comments.
  std::unordered_map<std::string, TermInfo5> term_set;

  size_t train_docs = 25000;

  // Stores the maximum frequency for every positive document.
  std::vector<size_t> good_docs_freq;

  // Stores the maximum frequency for every negative document.
  std::vector<size_t> bad_docs_freq;

  // Stores every unique term from the positive (negative) documents,
  // along with information about the average weight of the term, and
  // the documents it is found in. For more info, refer to the TermInfo
  // comments.
  std::unordered_map<std::string, TermInfo4> good_terms;
  std::unordered_map<std::string, TermInfo4> bad_terms;

  // The good (bad) vector has size the size of the term_set, and contains
  // the weight for every term, related to the good (bad) documents.
  std::vector<float> good_vector;
  std::vector<float> bad_vector;

  // Stores the terms for each document.
  std::vector<std::unordered_set<std::string>> good_docs_terms;
  std::vector<std::unordered_set<std::string>> bad_docs_terms;

  size_t knn = 3;
};

#endif