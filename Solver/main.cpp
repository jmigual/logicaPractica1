#include <iostream>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <chrono>
using namespace std;

#define UNDEF   -1
#define TRUE    1
#define FALSE   0

const int MAX_DECISIONS = 3500;

typedef unsigned int uint;
typedef unsigned long ulong;

uint numVars;
uint numClauses;
vector< vector<int> > clauses;  // Has the expressions
vector<short> model;            // Contains the current result
vector<int> modelStack;         // Contains the recursion and decisions
uint indexOfNextLitToPropagate; // Next index to be propagated
uint decisionLevel;             // DL

vector< int > litS;                   // Literal score
vector< vector<int> > clausesLitPos;  // Clauses where the literal is Positive
vector< vector<int> > clausesLitNeg;  // Clauses where the literal is Negative
long long totalPropagate = 0;
chrono::high_resolution_clock::time_point totalTime;

uint propCount = 0;
uint decisions = 0;
uint propagations = 0;
ulong conflicts = 0;
ulong lastCOrder = 0;
bool satisfiable = false;

// Literal comparator
bool litC(const int &a, const int &b) {
  return litS[a] > litS[b];
}

void readClauses() {
  // Skip comments
  char c = cin.get();
  while (c == 'c') {
    while (c != '\n') c = cin.get();
    c = cin.get();
  }
  
  // Read "cnf numVars numClauses"
  string aux;
  cin >> aux >> numVars >> numClauses;
  
  clauses.resize(numClauses);
  clausesLitPos.resize(numVars + 1);
  clausesLitNeg.resize(numVars + 1);
  litS = vector<int> (numVars + 1, 0);
  
  
  // Read clauses
  for (uint i = 0; i < numClauses; ++i) {
    int lit;
    while (cin >> lit and lit != 0) {
    
      if (lit > 0) clausesLitPos[lit].push_back(i);
      else clausesLitNeg[-lit].push_back(i);
      
      clauses[i].push_back(lit);
      ++litS[abs(lit)];
    }
  }
}

int currentValueInModel(int lit) {
  if (lit >= 0) return model[lit];
  else {
    if (model[-lit] == UNDEF) return UNDEF;
    else return 1 - model[-lit];
  }
}

void setLiteralToTrue(int lit) {
  modelStack.push_back(lit);
  if (lit > 0) model[lit] = TRUE;
  else model[-lit] = FALSE;
}

// Checks the selected clauses passed in the selected vector
bool checkClausulesForConflict ( vector<int> &selectedClauses) {
  for (uint i = 0; i < selectedClauses.size(); ++i) {
    int numClause = selectedClauses[i];
    
    bool someLitTrue = false;
    int numUndefs = 0;
    int lastLitUndef = 0;
    for (uint k = 0; not someLitTrue and k < clauses[numClause].size(); ++k) {
      int val = currentValueInModel(clauses[numClause][k]);
      if (val == TRUE) someLitTrue = true;
      else if (val == UNDEF) {
        ++numUndefs;
        lastLitUndef = clauses[numClause][k];
      }
    }
    if (not someLitTrue and numUndefs == 0) return true; // conflict! all lits false
    else if (not someLitTrue and numUndefs == 1) setLiteralToTrue(lastLitUndef);
  }
  return false;
}

bool propagateGivesConflict () {
  auto start = chrono::high_resolution_clock::now();
  bool ret = false;
  int lit = 0;
  
  while ( indexOfNextLitToPropagate < modelStack.size() && !ret ) {
    ++propagations;
    lit = modelStack[indexOfNextLitToPropagate];
    ++indexOfNextLitToPropagate;
    // Checks only the clauses where the Literal is assigned false
    if ((lit > 0 && checkClausulesForConflict(clausesLitNeg[lit])) ||
        (lit < 0 && checkClausulesForConflict(clausesLitPos[-lit]))) {
      ret = true;
    }
  }
  
  // Get time
  auto elapsed = chrono::high_resolution_clock::now() - start;
  totalPropagate += chrono::duration_cast<chrono::microseconds> (elapsed).count();
  
  // Penalize conflict
  if (ret) {
    lit = abs(lit);
    ++conflicts;
    litS[lit] += 1;
  }
  
  return ret;
}

void backtrack() {
  uint i = modelStack.size() - 1;
  int lit = 0;
  while (modelStack[i] != 0) { // 0 is the DL mark
    lit = modelStack[i];
    model[abs(lit)] = UNDEF;
    modelStack.pop_back();
    --i;
  }
  // at this point, lit is the last decision
  modelStack.pop_back(); // remove the DL mark
  --decisionLevel;
  indexOfNextLitToPropagate = modelStack.size();
  setLiteralToTrue(-lit);  // reverse last decision
}

// Heuristic for finding the next decision literal:
int getNextDecisionLiteral() {
  if (decisions % MAX_DECISIONS == 0) for (int &s : litS) s /= 2;
  
  int maxLit = 0;
  bool first = true;
  
  // Return the literal with the highest score and is not defined yet
  for (uint i = 1; i <= numVars; ++i) {
    if (first) {
      if (model[i] == UNDEF) {
        maxLit = i;
        first = false;
      }
    }
    else {
      if (model[i] == UNDEF && litS[i] > litS[maxLit]) maxLit = i;
    }    
  }
  return maxLit; // reurns 0 when all literals are defined
}

void checkmodel() {
  for (uint i = 0; i < numClauses; ++i) {
    bool someTrue = false;
    for (uint j = 0; not someTrue and j < clauses[i].size(); ++j) {
      someTrue = (currentValueInModel(clauses[i][j]) == TRUE);
    }
    if (not someTrue) {
      cout << "Error in model, clause is not satisfied:";
      for (uint j = 0; j < clauses[i].size(); ++j) cout << clauses[i][j] << " ";
      cout << endl;
      exit(1);
    }
  }
}

void printResults() {
  cout << (satisfiable ? "SATISFIABLE" : "UNSATISFIABLE" )  << endl;
  
  cout.setf(ios::fixed);
  cout.precision(2);
  
  auto elapsed = chrono::high_resolution_clock::now() - totalTime;
  long long tTime = chrono::duration_cast<chrono::microseconds> (elapsed).count();
  cerr << "Total Propagate: " << totalPropagate / 1000.0 << " ms "
       << totalPropagate / double(tTime) * 100.0 << "%" <<  endl;
  cout << "Total Time     : ";
  cout << tTime / 1000.0 << " ms" << endl;
  
  cout << endl;
  
  cerr << "Conflicts      : " << conflicts << endl;
  cout << "Decisions      : " << decisions << endl;
  cout << "Propagations   : " << propagations << endl;
  cout << "Propagations/s : " << propagations / (totalPropagate / 1.0e6) << endl;
}

int main() {
  totalTime = chrono::high_resolution_clock::now();
  
  readClauses(); // reads numVars, numClauses and clauses
  model.resize(numVars + 1, UNDEF);
  indexOfNextLitToPropagate = 0;
  decisionLevel = 0;
  
  // Take care of initial unit clauses, if any
  for (uint i = 0; i < numClauses; ++i) {
    if (clauses[i].size() == 1) {
      int lit = clauses[i][0];
      
      int val = currentValueInModel(lit);
      if (val == FALSE) {
        printResults();
        return 10;
      } else if (val == UNDEF) setLiteralToTrue(lit);
    }
  }
  
  // DPLL algorithm
  while (true) {
    while ( propagateGivesConflict() ) {
      if (decisionLevel == 0) {
        printResults();
        return 10;
      }
      backtrack();
    }
    
    int decisionLit = getNextDecisionLiteral();
    if (decisionLit == 0) {
      checkmodel();
      satisfiable = true;
      printResults();
      return 20;
    }
    
    // start new decision level:
    modelStack.push_back(0);          // push mark indicating new DL
    ++indexOfNextLitToPropagate;
    ++decisions;
    ++decisionLevel;
    setLiteralToTrue(decisionLit);    // now push decisionLit on top of the mark
  }
}

