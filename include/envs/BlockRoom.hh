/*
 * BlockRoom.hh
 *
 *  Created on: 14 févr. 2017
 *      Author: pierre
 */

#ifndef INCLUDE_ENVS_BLOCKROOM_HH_
#define INCLUDE_ENVS_BLOCKROOM_HH_

#include "../common/Random.h"
#include "../common/core.hh"
#include <random>
#include <algorithm>



class BlockRoom: public Environment {
public:
	// Constructor

	BlockRoom(Random &rand, bool with_tutor, bool stochastic, float finalReward, int nbRedBlocks, int nbBlueBlocks);

	/* Blockroom is not supposed to become a base class so no need
	 * for a virtual constructor ? TO be changed if so.
	 */
	~BlockRoom();


	void apply_tutor(int action);
	bool terminal() const;
	void reset();
	int getNumActions();
	int trueBestAction();
	int getNumTutorActions();
	std::vector<float> generate_state();
	float getEuclidianDistance(std::vector<float> & s1, std::vector<float> & s2,
			std::vector<float> minValues, std::vector<float>maxValues);
	std::pair<std::vector<float>,float> getMostProbNextState(std::vector<float> s, int action);
	void getMinMaxFeatures(std::vector<float> *minFeat, std::vector<float> *maxFeat);

	void getMinMaxReward(float* minR, float* maxR);
	bool isEpisodic(){ return false;}

	friend std::ostream &operator<<(std::ostream &out, const BlockRoom &blockroom);

	/** Prints the current map. */
	void print_map() const;

	int height;
	int width;
	int nbRedBlocks;
	int nbBlueBlocks;
	bool stochastic;
	Random &rng;
	bool WITH_TUTOR;
	bool tutor_attentive = true;
	int state_dim_base;
	std::vector<float> s;
	float finalReward;
	enum color{
		RED,
		BLUE
	};

	int get_blocks_in() const;
	int get_blocks_right() const;
	void tutorStop();

	std::default_random_engine engine;



	occ_info_t apply(int action);
	float getStateActionInfoError(std::vector<float> s, std::vector<StateActionInfo> preds);

	struct block_t{
		float* ns;
		float* ew;
		float* color;
		float* is_in_blue_box;
		float* is_in_red_box;
		block_t(float* a, float* b, float* c,
				float* e, float* f){
			ns = a;
			ew = b;
			color = c;
			is_in_blue_box = e;
			is_in_red_box = f;
		}
	};


	float* agent_ns;
	float* agent_ew;
	float* block_hold;
	float* agent_eye_ns;
	float* agent_eye_ew;
	float* red_box_ns;
	float* red_box_ew;
	float* blue_box_ns;
	float* blue_box_ew;
	std::vector<block_t> blocks;

	std::vector<float> t_state;
	float* tutor_eye_ns;
	float* tutor_eye_ew;


	std::map<std::string, int> tutor_actions;
	std::map<std::string, int> actions;
	int numactions;
	int num_tutor_actions;


	std::map<int, std::string> action_names;

	std::map<int, std::string> get_action_names();

	const std::vector<float> &sensation() const;
	int applyNoise(int action);
	std::vector<std::pair<int,int>> get_nearby_pos(int, int);
	bool isSyncTutor(std::vector<float>) const;
	std::vector<int> find_red_block_under_hand();
	std::vector<int> find_blue_block_under_hand();
	std::vector<int> find_block_under_eye();
	bool eye_hand_sync();
	void setDebug(bool b);
	void setVerbose(bool b);
	tutor_feedback tutorAction();


	bool BRDEBUG = false;
	bool IS_REAL = true;
	bool NOPICKBACK = true;
	int numstep;
	std::map<int, std::list<int>> actions_occurences;

};



#endif /* INCLUDE_ENVS_BLOCKROOM_HH_ */
