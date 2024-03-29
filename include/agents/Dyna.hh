/** \file
    Interface for an implementation of the canonical Q-learning
    algorithm. */

#ifndef _DYNA_HH_
#define _DYNA_HH_

#include "../common/Random.h"
#include "../common/core.hh"

#include <map>
#include <set>
#include <vector>

/** Agent that uses straight Q-learning, with no generalization and
    epsilon-greedy exploration. */
class Dyna: public Agent {
public:
  /** Standard constructor
      \param numactions The number of possible actions
      \param gamma The discount factor
      \param initialvalue The initial value of each Q(s,a)
      \param alpha The learning rate
      \param epsilon The probability of taking a random action
      \param rng Initial state of the random number generator to use */
  Dyna(int numactions, float gamma,
       float initialvalue, float alpha, int k, float epsilon,
	   Random rng = Random());

  /** Unimplemented copy constructor: internal state cannot be simply
      copied. */
  Dyna(const Dyna &);

  virtual ~Dyna();

  virtual int first_action(const std::vector<float> &s);
  virtual int next_action(float r, const std::vector<float> &s);
  virtual void last_action(float r);
  virtual void setDebug(bool d);
  virtual void seedExp(std::vector<experience>);
  virtual void savePolicy(const char* filename);

  
  int getBestAction(const std::vector<float> &s);
  double getSeconds();

  void printState(const std::vector<float> &s);
  float getValue(std::vector<float> state);
  
  std::vector<float>::iterator random_max_element(
						   std::vector<float>::iterator start,
						   std::vector<float>::iterator end);

  void logValues(ofstream *of, int xmin, int xmax, int ymin, int ymax);
  float epsilon;

protected:
  /** The implementation maps all sensations to a set of canonical
      pointers, which serve as the internal representation of
      environment state. */
  typedef const std::vector<float> *state_t;

  struct dynaExperience {
    state_t s;
    state_t next;
    float r;
    bool term;
    int a;
  };
  void addExperience(float r, state_t s, bool term);

  /** Produces a canonical representation of the given sensation.
      \param s The current sensation from the environment.
      \return A pointer to an equivalent state in statespace. */
  state_t canonicalize(const std::vector<float> &s);

private:
  /** Set of all distinct sensations seen.  Pointers to elements of
      this set serve as the internal representation of the environment
      state. */
  std::set<std::vector<float> > statespace;

  /** The primary data structure of the learning algorithm, the value
      function Q.  For state_t s and int a, Q[s][a] gives the
      learned maximum expected future discounted reward conditional on
      executing action a in state s. */
  std::map<state_t, std::vector<float> > Q;

  std::vector<dynaExperience> experiences;

  const int numactions;
  const float gamma;

  const float initialvalue;
  const float alpha;
  const int k;

  Random rng;
  float *currentq;
  state_t laststate;
  int lastact;

  bool ACTDEBUG;
};

#endif
