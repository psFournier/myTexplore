/*
 * BlockRoom.cc
 *
 *  Created on: 14 févr. 2017
 *      Author: pierre
 */

#include "../../include/envs/BlockRoom.hh"
#include <algorithm>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>


using namespace cv;

BlockRoom::BlockRoom(Random &rand, bool with_tutor, bool stochastic, float finalReward, int nbRedBlocks, int nbBlueBlocks):
			height(6),
			width(6),
			nbRedBlocks(nbRedBlocks),
			nbBlueBlocks(nbBlueBlocks),
			stochastic(stochastic),
			rng(rand),
			WITH_TUTOR(with_tutor),
			state_dim_base(9),
			s(state_dim_base+5*(nbRedBlocks+nbBlueBlocks)),
			finalReward(finalReward),
			agent_ns(&(s[0])),
			agent_ew(&(s[1])),
			block_hold(&(s[2])),
			agent_eye_ns(&(s[3])),
			agent_eye_ew(&(s[4])),
			red_box_ns(&(s[5])),
			red_box_ew(&(s[6])),
			blue_box_ns(&(s[7])),
			blue_box_ew(&(s[8])),
			t_state(2),
			numstep(0)
{
	int cnt_actions = 0;
	int cnt_tutor_actions = 0;

//	actions[std::string("NORTH")] = cnt_actions++;
//	actions[std::string("SOUTH")] = cnt_actions++;
//	actions[std::string("EAST")] = cnt_actions++;
//	actions[std::string("WEST")] = cnt_actions++;
	actions[std::string("GO_TO_EYE")] = cnt_actions++;
	actions[std::string("LOOK_RED_BOX")] = cnt_actions++;
	actions[std::string("LOOK_BLUE_BOX")] = cnt_actions++;
	/*actions[std::string("PICK_BLUE")] = cnt_actions++;
	actions[std::string("PICK_RED")] = cnt_actions++;*/
	actions[std::string("PICK")] = cnt_actions++;
//	actions[std::string("PUT_DOWN")] = cnt_actions++;
	actions[std::string("PUT_IN")] = cnt_actions++;


	if (WITH_TUTOR){
		tutor_eye_ns = &(t_state[0]);
		tutor_eye_ew = &(t_state[1]);

		tutor_actions[std::string("LOOK_AGENT")] = cnt_tutor_actions++;
		tutor_actions[std::string("LOOK_RED_BOX")] = cnt_tutor_actions++;
		tutor_actions[std::string("LOOK_BLUE_BOX")] = cnt_tutor_actions++;
	}

	int nb_fix_actions = cnt_actions;

	for (int i = 0; i<nbRedBlocks; i++){
		block_t block(
				&(s[5*i+state_dim_base + 0]),
				&(s[5*i+state_dim_base + 1]),
				&(s[5*i+state_dim_base + 2]),
				&(s[5*i+state_dim_base + 3]),
				&(s[5*i+state_dim_base + 4]));
		blocks.push_back(block);

		std::string name = "LOOK_RED_BLOCK_";
		name += std::to_string(i);
		actions[name] = cnt_actions++;
		if (WITH_TUTOR){
			tutor_actions[name] = cnt_tutor_actions++;
		}

	}

	for (int i = 0; i<nbBlueBlocks; i++){
		block_t block(
				&(s[5*(i+nbRedBlocks)+state_dim_base + 0]),
				&(s[5*(i+nbRedBlocks)+state_dim_base + 1]),
				&(s[5*(i+nbRedBlocks)+state_dim_base + 2]),
				&(s[5*(i+nbRedBlocks)+state_dim_base + 3]),
				&(s[5*(i+nbRedBlocks)+state_dim_base + 4]));
		blocks.push_back(block);

		std::string name = "LOOK_BLUE_BLOCK_";
		name += std::to_string(i);
		actions[name]= cnt_actions++;
		if (WITH_TUTOR){
			tutor_actions[name] = cnt_tutor_actions++;
		}
	}

	numactions = cnt_actions;
	num_tutor_actions = cnt_tutor_actions;
	for (std::map<std::string, int>::iterator it = actions.begin();
			it != actions.end() ; ++it ){
		action_names[it->second] = it->first;
	}

	*agent_ns = rng.uniformDiscrete(0, height-1);
	*agent_ew = rng.uniformDiscrete(0, width-1);
	*agent_eye_ns = rng.uniformDiscrete(0, height-1);
	*agent_eye_ew = rng.uniformDiscrete(0, width-1);
	reset();

	if (BRDEBUG) print_map();
}

BlockRoom::~BlockRoom() {}

const std::vector<float> &BlockRoom::sensation() const {
	if (BRDEBUG) print_map();
	return s;
}

bool BlockRoom::terminal() const {
	return get_blocks_in()==nbBlueBlocks+nbRedBlocks;
}

std::map<int, std::string> BlockRoom::get_action_names(){
	return action_names;
}

bool BlockRoom::isSyncTutor(std::vector<float> state) const {
	return (state[0]==*tutor_eye_ns && state[1]==*tutor_eye_ew && tutor_attentive);
}
int BlockRoom::get_blocks_in() const {
	int nb_blocks_in = 0;
	for (auto block: blocks){
		nb_blocks_in += (*block.is_in_blue_box || *block.is_in_red_box);
	}
	return nb_blocks_in;
}

int BlockRoom::get_blocks_right() const {
	int nb_blocks_right = 0;
	for (auto block: blocks){
		nb_blocks_right += (*block.is_in_blue_box && *block.color==1) || (*block.is_in_red_box && *block.color==0);
	}
	return nb_blocks_right;
}


int BlockRoom::getNumActions() {
	if (BRDEBUG) cout << "Return number of actions: " << numactions << endl;
	return numactions; //num_actions;
}

int BlockRoom::getNumTutorActions() {
	if (BRDEBUG) cout << "Return number of tutor actions: " << num_tutor_actions << endl;
	return num_tutor_actions; //num_actions;
}

void BlockRoom::getMinMaxFeatures(std::vector<float> *minFeat,
		std::vector<float> *maxFeat){

	minFeat->resize(s.size(), 0.0);
	float maxSize = height > width ? height : width;
	maxFeat->resize(s.size(), maxSize-1);

	(*maxFeat)[0] = height-1;
	(*maxFeat)[1] = width-1;
	(*maxFeat)[2] = nbRedBlocks+nbBlueBlocks-1;
	(*maxFeat)[3] = height-1;
	(*maxFeat)[4] = width-1;
	(*maxFeat)[5] = height-1;
	(*maxFeat)[6] = width-1;
	(*maxFeat)[7] = height-1;
	(*maxFeat)[8] = width-1;

	for (int i = 0; i<nbRedBlocks+nbBlueBlocks; i++){
		(*maxFeat)[state_dim_base+5*i] = height-1;
		(*maxFeat)[state_dim_base+5*i+1] = width-1;
		(*maxFeat)[state_dim_base+5*i+2] = 1;
		(*maxFeat)[state_dim_base+5*i+3] = 1;
		(*maxFeat)[state_dim_base+5*i+4] = 1;
	}

	(*minFeat)[2] = -1;
}

void BlockRoom::getMinMaxReward(float *minR,
		float *maxR){

	*minR = 0.0;
	*maxR = 100.0;

}

void BlockRoom::print_map() const{

	int blockSize=80;
	Size size(blockSize, blockSize);

	std::map<std::pair<int,int>,std::list<Mat>> posToImg;

	Mat chessBoard(blockSize*height,blockSize*width,CV_8UC3,Scalar::all(0));
	unsigned char color=0;
	string img_dir = "/home/pierre/workspace/myTexplore/images/";
	Mat red_block_img = imread(img_dir+"red_block.png",CV_LOAD_IMAGE_COLOR);
	resize(red_block_img, red_block_img, size);
	Mat blue_block_img = imread(img_dir+"blue_block.png",CV_LOAD_IMAGE_COLOR);
	resize(blue_block_img, blue_block_img, size);
	Mat blue_box_img = imread(img_dir+"blue_box.png",CV_LOAD_IMAGE_COLOR);
	resize(blue_box_img, blue_box_img, size);
	Mat red_box_img = imread(img_dir+"red_box.png",CV_LOAD_IMAGE_COLOR);
	resize(red_box_img, red_box_img, size);
	Mat agent_eye_img = imread(img_dir+"agent_eye.png",CV_LOAD_IMAGE_COLOR);
	resize(agent_eye_img, agent_eye_img, size);
	Mat tutor_eye_img = imread(img_dir+"tutor_eye.png",CV_LOAD_IMAGE_COLOR);
	resize(tutor_eye_img, tutor_eye_img, size);
	Mat agent_hand_img = imread(img_dir+"agent_hand.png",CV_LOAD_IMAGE_COLOR);
	resize(agent_hand_img, agent_hand_img, size);



	for(int i=0;i<blockSize*height;i=i+blockSize){
		color=~color;
		for(int j=0;j<blockSize*width;j=j+blockSize){
			Mat ROI=chessBoard(Rect(i,j,blockSize,blockSize));
			ROI.setTo(Scalar::all(color));
			color=~color;
		}
	}

	for (std::vector<block_t>::const_iterator it = blocks.begin();  it != blocks.end(); ++it){
		if (*(it->color)==RED){
			int x = blockSize*(*(it->ew));
			int y = blockSize*(*(it->ns));
			posToImg[std::pair<int,int>(x,y)].push_back(red_block_img);
			/*red_block_img.copyTo(chessBoard(cv::Rect(blockSize*(*(it->ns)),
					blockSize*(*(it->ew)),red_block_img.cols, red_block_img.rows)));*/
		}
		if (*(it->color)==BLUE){
			int x = blockSize*(*(it->ew));
			int y = blockSize*(*(it->ns));
			posToImg[std::pair<int,int>(x,y)].push_back(blue_block_img);
			/*blue_block_img.copyTo(chessBoard(cv::Rect(blockSize*(*(it->ns)),
					blockSize*(*(it->ew)),blue_block_img.cols, blue_block_img.rows)));*/
		}
	}

	posToImg[std::pair<int,int>(blockSize*(*blue_box_ew),blockSize*(*blue_box_ns))].push_back(blue_box_img);
	posToImg[std::pair<int,int>(blockSize*(*red_box_ew),blockSize*(*red_box_ns))].push_back(red_box_img);
	posToImg[std::pair<int,int>(blockSize*(*agent_eye_ew),blockSize*(*agent_eye_ns))].push_back(agent_eye_img);
	if (WITH_TUTOR){
		posToImg[std::pair<int,int>(blockSize*(*tutor_eye_ew),blockSize*(*tutor_eye_ns))].push_back(tutor_eye_img);
	}
	posToImg[std::pair<int,int>(blockSize*(*agent_ew),blockSize*(*agent_ns))].push_back(agent_hand_img);

	for (auto elem : posToImg){
		if (elem.second.size()==1){
			elem.second.front().copyTo(chessBoard(cv::Rect(elem.first.first,
					elem.first.second,elem.second.front().cols, elem.second.front().rows)));
		}
		if (elem.second.size()>1 && elem.second.size()<5){
			int i = 0;
			while (!elem.second.empty()){
				Size size2(blockSize/2, blockSize/2);
				resize(elem.second.front(), elem.second.front(), size2);
				int x_decal = i % 2;
				int y_decal = i/2;
				elem.second.front().copyTo(chessBoard(cv::Rect(elem.first.first+x_decal*blockSize/2,
						elem.first.second+y_decal*blockSize/2,
						elem.second.front().cols,
						elem.second.front().rows)));
				elem.second.pop_front();
				i++;
			}
		}
		else if (elem.second.size()>4) {
			int i = 0;
			while (!elem.second.empty()){
				Size size3(blockSize/4, blockSize/4);
				resize(elem.second.front(), elem.second.front(), size3);
				int x_decal = i % 4;
				int y_decal = i/4;
				elem.second.front().copyTo(chessBoard(cv::Rect(elem.first.first+x_decal*blockSize/4,
						elem.first.second+y_decal*blockSize/4,
						elem.second.front().cols,
						elem.second.front().rows)));
				elem.second.pop_front();
				i++;
			}
		}
	}

	imshow("Chess board", chessBoard);
	waitKey(1);
}

std::vector<float> BlockRoom::generate_state(){
	/* Beware, not working because of with_tutor
	std::vector<float> g(state_dim_base+5*(nbRedBlocks+nbBlueBlocks)+2*WITH_TUTOR);
	g[6] = rng.uniformDiscrete(0, width-1);
	g[5] = rng.uniformDiscrete(0, height-1);
	g[2] = -1;
	do {
		g[8] = rng.uniformDiscrete(0, width-1);
		g[7] = rng.uniformDiscrete(0, height-1);
	} while ((*red_box_ew)==(*blue_box_ew) && (*blue_box_ns)==(*blue_box_ns));
	g[0] = rng.uniformDiscrete(0, height-1);
	g[1] = rng.uniformDiscrete(0, width-1);
	g[4] = rng.uniformDiscrete(0, width-1);
	g[3] = rng.uniformDiscrete(0, height-1);

	bool hand_full = false;
	for (int i = 0; i<nbRedBlocks; i++){
		g[5*i+state_dim_base+2*WITH_TUTOR+2] = RED;
		g[5*i+state_dim_base+2*WITH_TUTOR+3] = 0;
		g[5*i+state_dim_base+2*WITH_TUTOR+4] = 0;
		float randProb = rng.uniform();
		float prob = 0.1;
		if (randProb < prob) {
			// Red block
			g[5*i+state_dim_base+2*WITH_TUTOR + 0] = g[5];
			g[5*i+state_dim_base+2*WITH_TUTOR + 1] = g[6];
			g[5*i+state_dim_base+2*WITH_TUTOR+4] = 1;
		}
		else {
			prob += 0.1;
			if (randProb<prob){
				// blue block
				g[5*i+state_dim_base+2*WITH_TUTOR + 0] = g[7];
				g[5*i+state_dim_base+2*WITH_TUTOR + 1] = g[8];
				g[5*i+state_dim_base+2*WITH_TUTOR+3] = 1;
			}
			else {
				prob += 0.2;
				if (randProb<prob && !hand_full){
					// hand
					g[5*i+state_dim_base+2*WITH_TUTOR + 0] = g[0];
					g[5*i+state_dim_base+2*WITH_TUTOR + 1] = g[1];
					g[2] = i;
					hand_full = true;
				}
				else {
					g[5*i+state_dim_base+2*WITH_TUTOR + 0] = rng.uniformDiscrete(0, height-1);
					g[5*i+state_dim_base+2*WITH_TUTOR + 1] = rng.uniformDiscrete(0, width-1);
				}
			}
		}
	}
	for (int i = 0; i<nbBlueBlocks; i++){
		g[5*(i+nbRedBlocks)+state_dim_base+2*WITH_TUTOR+2] = BLUE;
		g[5*(i+nbRedBlocks)+state_dim_base+2*WITH_TUTOR+3] = 0;
		g[5*(i+nbRedBlocks)+state_dim_base+2*WITH_TUTOR+4] = 0;
		float randProb = rng.uniform();
		float prob = 0.1;
		if (randProb < prob) {
			// Red block
			g[5*(i+nbRedBlocks)+state_dim_base+2*WITH_TUTOR + 0] = g[5];
			g[5*(i+nbRedBlocks)+state_dim_base+2*WITH_TUTOR + 1] = g[6];
			g[5*(i+nbRedBlocks)+state_dim_base+2*WITH_TUTOR+4] = 1;
		}
		else {
			prob += 0.1;
			if (randProb<prob){
				// blue block
				g[5*(i+nbRedBlocks)+state_dim_base+2*WITH_TUTOR + 0] = g[7];
				g[5*(i+nbRedBlocks)+state_dim_base+2*WITH_TUTOR + 1] = g[8];
				g[5*(i+nbRedBlocks)+state_dim_base+2*WITH_TUTOR+3] = 1;
			}
			else {
				prob += 0.2;
				if (randProb<prob && !hand_full){
					// hand
					g[5*(i+nbRedBlocks)+state_dim_base+2*WITH_TUTOR + 0] = g[0];
					g[5*(i+nbRedBlocks)+state_dim_base+2*WITH_TUTOR + 1] = g[1];
					g[2] = i;
					g[2] = i;
					hand_full = true;
				}
				else {
					g[5*(i+nbRedBlocks)+state_dim_base+2*WITH_TUTOR + 0] = rng.uniformDiscrete(0, height-1);
					g[5*(i+nbRedBlocks)+state_dim_base+2*WITH_TUTOR + 1] = rng.uniformDiscrete(0, width-1);
				}
			}
		}
	}
	return g;*/
}

void BlockRoom::reset(){
	(*block_hold) = -1;

	*(blocks[0].ew) = 4;
	*(blocks[0].ns) = 4;
	*(blocks[0].color) = 0;
	*(blocks[0].is_in_blue_box) = false;
	*(blocks[0].is_in_red_box) = false;

	*(blocks[1].ew) = 3;
	*(blocks[1].ns) = 3;
	*(blocks[1].color) = 0;

	*(blocks[1].is_in_blue_box) = false;
	*(blocks[1].is_in_red_box) = false;

	*(blocks[2].ew) = 2;
	*(blocks[2].ns) = 2;
	*(blocks[2].color) = 1;

	*(blocks[2].is_in_blue_box) = false;
	*(blocks[2].is_in_red_box) = false;


	*blue_box_ew = 0;
	*blue_box_ns = 0;

	*red_box_ew = 0;
	*red_box_ns = 4;



}

int BlockRoom::applyNoise(int action){
	return action;
}

void BlockRoom::setDebug(bool b){
	BRDEBUG = b;
}

void BlockRoom::setVerbose(bool b){
	IS_REAL = b;
}

std::vector<int> BlockRoom::find_block_under_eye() {
	std::vector<int> l;
	int cnt = 0;
	for (auto block: blocks){
		if (*(block.ns)==*agent_eye_ns && *(block.ew)==*agent_eye_ew){
			if (!NOPICKBACK || (!(*(block.is_in_blue_box))&& !(*(block.is_in_red_box)))){
				l.push_back(cnt);
			}
		}
		cnt++;
	}
	return(l);
}

std::vector<int> BlockRoom::find_red_block_under_hand() {
	std::vector<int> l;
	for (std::vector<block_t>::iterator it = blocks.begin(); it != blocks.end(); ++it){
		if (*(it->ns)==(*agent_ns) && *(it->ew)==(*agent_ew) && *(it->color)==RED){
			l.push_back(it-blocks.begin());
		}
	}
	return(l);
}

std::vector<int> BlockRoom::find_blue_block_under_hand() {
	std::vector<int> l;
	for (std::vector<block_t>::iterator it = blocks.begin(); it != blocks.end(); ++it){
		if (*(it->ns)==(*agent_ns) && *(it->ew)==(*agent_ew) && *(it->color)==BLUE){
			l.push_back(it-blocks.begin());
		}
	}
	return(l);
}

bool BlockRoom::eye_hand_sync(){
	return (*agent_eye_ns==*agent_ns
			&& *agent_eye_ew==*agent_ew);
}
void BlockRoom::apply_tutor(int action){
	if (action==actions["LOOK_AGENT"]){
		(*tutor_eye_ew) = (*agent_eye_ew);
		(*tutor_eye_ns) = (*agent_eye_ns);
	}
	if (action==tutor_actions["LOOK_RED_BOX"])	{
		(*tutor_eye_ew) = (*red_box_ew);
		(*tutor_eye_ns) = (*red_box_ns);
	}
	if (action==tutor_actions["LOOK_BLUE_BOX"]){
		(*tutor_eye_ew) = (*blue_box_ew);
		(*tutor_eye_ns) = (*blue_box_ns);
	}
	if (action>num_tutor_actions-nbBlueBlocks-nbRedBlocks-1
			&& action<num_tutor_actions){
		int num_block = action-(num_tutor_actions-nbBlueBlocks-nbRedBlocks);
		if (!(*(blocks[num_block].is_in_blue_box))
				&& !(*(blocks[num_block].is_in_red_box))
				&& *(block_hold) != num_block){
			(*tutor_eye_ew) = *(blocks[num_block].ew);
			(*tutor_eye_ns) = *(blocks[num_block].ns);
		}
	}
}

std::vector<std::pair<int,int>> BlockRoom::get_nearby_pos(int ns, int ew){
	std::vector<std::pair<int,int>> nearby_pos;
	if (ns<height-1){nearby_pos.push_back(std::make_pair(ns+1,ew));}
	if (ew<width-1){nearby_pos.push_back(std::make_pair(ns,ew+1));}
	if (ew>0){nearby_pos.push_back(std::make_pair(ns,ew-1));}
	if (ns>0){nearby_pos.push_back(std::make_pair(ns-1,ew));}
	return nearby_pos;
}



float BlockRoom::getEuclidianDistance(std::vector<float> & s1, std::vector<float> & s2,
		std::vector<float> minValues, std::vector<float>maxValues){
	float res = 0.;
	unsigned nfeats = s1.size();
	std::vector<float> featRange(nfeats, 0);
	for (unsigned i = 0; i < nfeats; i++){
		featRange[i] = minValues[i] - maxValues[i];
	}
	if (s1.size()!=s2.size()){return -1;}
	for (int i=0; i<s1.size(); i++){
		res+=pow((s1[i]-s2[i])/featRange[i],2);
	}
	return sqrt(res/nfeats);
}

std::pair<std::vector<float>,float> BlockRoom::getMostProbNextState(std::vector<float> state, int action){
	/*std::vector<float> next_state = state;
	float reward = 0.;
	if (action==actions["GO_TO_EYE"]) {
		next_state[0] = state[3];
		next_state[1] = state[4];
		if (state[2]>=0){
			next_state[5*state[2]+state_dim_base+2*WITH_TUTOR]=state[3];
			next_state[5*state[2]+state_dim_base+2*WITH_TUTOR+1]=state[4];
		}
	}
	if (action == actions["PICK"]){
		if ((state[2])==-1 && state[3]==state[0] && state[4]==state[1]) {
			std::vector<int> blocks_under;
			for (int i=0; i<nbRedBlocks+nbBlueBlocks; i++){
				if ((state[5*i+state_dim_base+2*WITH_TUTOR] == state[0] &&
						state[5*i+state_dim_base+2*WITH_TUTOR+1] == state[1]))
				{
					if (!NOPICKBACK || (state[5*i+state_dim_base+2*WITH_TUTOR+3]==0 &&
							state[5*i+state_dim_base+2*WITH_TUTOR+4]==0)){
						blocks_under.push_back(i);
					}
				}
			}
			if (!blocks_under.empty()) {
				int block = blocks_under.back();
				next_state[2] = block;
				next_state[5*block+state_dim_base+2*WITH_TUTOR + 3] = 0;
				next_state[5*block+state_dim_base+2*WITH_TUTOR + 4] = 0;
				//reward += 10;
			}
		}
	}
	if (action==actions["PUT_DOWN"]) {
		if (state[2]!=-1
				&& state[3]==state[0] && state[4]==state[1]
										 && find_block_under(state[0],state[1]).empty()
										 && ((state[5])!=(state[0]) || (state[6])!=(state[1]))
										 && ((state[7])!=(state[0]) || (state[8])!=(state[1]))){
			next_state[2]=-1;
		}
	}
	if (action==actions["PUT_IN"]) {
		if ((state[2])!=-1){
			if ((state[5])==(state[0]) && (state[6])==(state[1])){
				next_state[5*state[2]+state_dim_base+2*WITH_TUTOR+4] = 1;
				next_state[2]=-1;
				if (get_blocks_in(next_state)==nbRedBlocks+nbBlueBlocks){
					reward += 1000;
				}
				else {
					if (NOPICKBACK) reward += 100;
				}
			}
			else if ((state[7])==(state[0]) && (state[8])==(state[1])){
				next_state[5*state[2]+state_dim_base+2*WITH_TUTOR+3] = 1;
				next_state[2]=-1;
				if (get_blocks_in(next_state)==nbRedBlocks+nbBlueBlocks){
					reward += 1000;
				}
				else {
					if (NOPICKBACK) reward += 100;
				}
			}
		}
	}
	if (WITH_TUTOR && action==actions["LOOK_TUTOR"]){
		if ((state[4]) != (state[10]) || (state[3]) != (state[9])){
			next_state[4] = (state[10]);
			next_state[3] = (state[9]);
		}
	}
	if (action==actions["LOOK_RED_BOX"]){
		next_state[4] = (state[6]);
		next_state[3] = (state[5]);
	}
	if (action==actions["LOOK_BLUE_BOX"]){
		next_state[4] = (state[8]);
		next_state[3] = (state[7]);
	}
	if (action>numactions-nbBlueBlocks-nbRedBlocks-1
			&& action<numactions){
		int num_block = action-(numactions-nbBlueBlocks-nbRedBlocks);
		if (state[2]!=num_block && state[5*num_block+state_dim_base+2*WITH_TUTOR+3]==0
				&& state[5*num_block+state_dim_base+2*WITH_TUTOR+4]==0) {
			next_state[4] = state[5*num_block+state_dim_base+2*WITH_TUTOR+1];
			next_state[3] = state[5*num_block+state_dim_base+2*WITH_TUTOR];
		}
	}
	return std::make_pair(next_state,reward);*/
}




float BlockRoom::getStateActionInfoError(std::vector<float> s, std::vector<StateActionInfo> preds){}

occ_info_t BlockRoom::apply(int action){
	float reward = 0.;
	float virtual_reward = 0;
	bool success = false;
	float stoch_param = (stochastic ? 0.8 : 1.);

	/*
	if (action==actions["NORTH"]) {
		if ((*agent_ns) < height-1) {
			(*agent_ns)++;
			success = true;
		}
	}
	if (action==actions["SOUTH"]) {
		if ((*agent_ns) > 0) {
			(*agent_ns)--;
			success = true;
		}
	}
	if (action==actions["EAST"]) {
		if ((*agent_ew) < width-1) {
			(*agent_ew)++;
			success = true;
		}
	}
	if (action==actions["WEST"]) {
		if ((*agent_ew) > 0) {
			(*agent_ew)--;
			success = true;
		}
	}
	*/

	if (action==actions["GO_TO_EYE"]) {
		if (rng.bernoulli(stoch_param)) {
			(*agent_ns) = (*agent_eye_ns);
			(*agent_ew) = (*agent_eye_ew);
		}
		else {
			std::vector<std::pair<int,int>> nearby_pos = get_nearby_pos(*agent_eye_ns,*agent_eye_ew);
			std::shuffle(nearby_pos.begin(), nearby_pos.end(), engine);
			(*agent_ns) = nearby_pos.front().first;
			(*agent_ew) = nearby_pos.front().second;
		}
		success = true;
		//reward -= 1;
	}

	if (action == actions["PICK"]){
		if (((*block_hold)==-1) && (eye_hand_sync())) {
			std::vector<int> blocks_under = find_block_under_eye();
			if (!blocks_under.empty()) {
				//std::shuffle(blocks_under.begin(), blocks_under.end(), engine);
				int idx = blocks_under.back();
				if (rng.bernoulli(stoch_param)){
					(*block_hold) = idx;
					*(blocks[idx].is_in_blue_box) = false;
					*(blocks[idx].is_in_red_box) = false;
				}
				success = true;
				if (IS_REAL) {
					std::cout << "block " << idx <<" taken." << std::endl;
				}
				//reward -= 1;
				//reward += 10;
			}
		}
	}
	/*if (action==actions["PICK_BLUE"]) {
		if ((*block_hold)==-1 && eye_hand_sync()) {
			std::vector<int> blue_blocks_under = find_blue_block_under_hand();
			if (!blue_blocks_under.empty()) {
				std::shuffle(blue_blocks_under.begin(), blue_blocks_under.end(), engine);
				int idx = blue_blocks_under.back();
	 *(blocks[idx].is_in_robot_hand) = true;
				(*block_hold) = idx;
				success = true;
			}
		}
	}
	if (action==actions["PICK_RED"]) {
		if ((*block_hold)==-1 && eye_hand_sync()) {
			std::vector<int> red_blocks_under = find_red_block_under_hand();
			if (!red_blocks_under.empty()) {
				std::shuffle(red_blocks_under.begin(), red_blocks_under.end(), engine);
				int idx = red_blocks_under.back();
	 *(blocks[idx].is_in_robot_hand) = true;
				(*block_hold) = idx;
				success = true;
			}
		}
	}*/
	/*if (action==actions["PUT_DOWN"]) {
		std::vector<int> red_blocks_under = find_red_block_under_hand();
		std::vector<int> blue_blocks_under = find_blue_block_under_hand();
		if (((*block_hold)!=-1)
				&& eye_hand_sync()
				&& red_blocks_under.empty()
				&& blue_blocks_under.empty()
				&& ((*red_box_ns)!=(*agent_ns) || (*red_box_ew)!=(*agent_ew))
				&& ((*blue_box_ns)!=(*agent_ns) || (*blue_box_ew)!=(*agent_ew))){
			if (rng.bernoulli(stoch_param)){
	 *(blocks[(*block_hold)].ns) = (*agent_ns);
	 *(blocks[(*block_hold)].ew) = (*agent_ew);
			}
			else{
				std::vector<std::pair<int,int>> nearby_pos = get_nearby_pos(*agent_ns,*agent_ew);
				std::shuffle(nearby_pos.begin(), nearby_pos.end(), engine);
	 *(blocks[(*block_hold)].ns) = nearby_pos.front().first;
	 *(blocks[(*block_hold)].ew) = nearby_pos.front().second;
			}

			(*block_hold) = -1;
			success = true;
		}
	}*/
	if (action==actions["PUT_IN"]) {
		if ((*block_hold)!=-1){
			if ((*red_box_ns)==(*agent_ns) && (*red_box_ew)==(*agent_ew)){
				if (rng.bernoulli(stoch_param)){
					*(blocks[(*block_hold)].is_in_red_box) = true;
					*(blocks[(*block_hold)].ns) = (*red_box_ns);
					*(blocks[(*block_hold)].ew) = (*red_box_ew);
				}
				else{
					std::vector<std::pair<int,int>> nearby_pos = get_nearby_pos(*red_box_ns,*red_box_ew);
					std::shuffle(nearby_pos.begin(), nearby_pos.end(), engine);
					*(blocks[(*block_hold)].ns) = nearby_pos.front().first;
					*(blocks[(*block_hold)].ew) = nearby_pos.front().second;
				}

				success = true;
				if (IS_REAL){
					std::cout << "block " << *block_hold <<" put in : "<< get_blocks_in() <<" blocks in boxes." << std::endl;
				}

				if (terminal()){
					reward += finalReward;
				}
				else {
					//if (NOPICKBACK) reward += 100;
				}
				(*block_hold) = -1;
			}
			else if ((*blue_box_ns)==(*agent_ns) && (*blue_box_ew)==(*agent_ew)){
				if (rng.bernoulli(stoch_param)){
					*(blocks[(*block_hold)].is_in_blue_box) = true;
					*(blocks[(*block_hold)].ns) = (*blue_box_ns);
					*(blocks[(*block_hold)].ew) = (*blue_box_ew);
				}
				else{
					std::vector<std::pair<int,int>> nearby_pos = get_nearby_pos(*blue_box_ns,*blue_box_ew);
					std::shuffle(nearby_pos.begin(), nearby_pos.end(), engine);
					*(blocks[(*block_hold)].ns) = nearby_pos.front().first;
					*(blocks[(*block_hold)].ew) = nearby_pos.front().second;
				}
				success = true;
				if (IS_REAL) {
					std::cout << "block " << *block_hold <<" put in : "<< get_blocks_in() <<" blocks in boxes." << std::endl;
				}
				if (terminal()){
					reward += finalReward;
				}
				else {
					//if (NOPICKBACK) reward += 100;
				}
				(*block_hold) = -1;

			}
		}
	}
	if (action==actions["LOOK_RED_BOX"])	{
		(*agent_eye_ew) = (*red_box_ew);
		(*agent_eye_ns) = (*red_box_ns);
		success = true;
		//reward -= 1;
	}
	if (action==actions["LOOK_BLUE_BOX"]){
		(*agent_eye_ew) = (*blue_box_ew);
		(*agent_eye_ns) = (*blue_box_ns);
		success = true;
		//reward -= 1;
	}
	if (action>numactions-nbBlueBlocks-nbRedBlocks-1
			&& action<numactions){
		int num_block = action-(numactions-nbBlueBlocks-nbRedBlocks);
		if (!(*(blocks[num_block].is_in_blue_box))
				&& !(*(blocks[num_block].is_in_red_box))
				&& *(block_hold)!=num_block){
			(*agent_eye_ew) = *(blocks[num_block].ew);
			(*agent_eye_ns) = *(blocks[num_block].ns);
			success = true;
			//reward -= 1;
		}
	}
	if ((*block_hold)>-1){
		*(blocks[(*block_hold)].ns) = (*agent_ns);
		*(blocks[(*block_hold)].ew) = (*agent_ew);
	}

	actions_occurences[action].push_back(numstep);
	numstep++;
	return occ_info_t(reward, success, get_blocks_in(), get_blocks_right(), );
}

/*int BlockRoom::trueBestAction(std::vector<float> &state){
	int res = -1;
	if (state[2] != -1) {
		if (((state[0]==state[5] && state[1]==state[6])||
				(state[0]==state[7] && state[1]==state[8])) && (state[0]==state[3] && state[1]==state[4])){
			res = 4;
		}
		else if ((state[3] != state[5] || state[4]!=state[6]) && (state[3]!=state[7]||state[4]!=state[8])){
			res = (rng.bernoulli(0.5) ? 1 : 2);
		}
	}
	else if (state[2]==-1){
		std::vector<int> blocks_under;
		for (int i=0; i<nbRedBlocks+nbBlueBlocks; i++){
			if ((state[5*i+state_dim_base+2*WITH_TUTOR] == state[0] &&
					state[5*i+state_dim_base+2*WITH_TUTOR+1] == state[1]))
			{
				if (!NOPICKBACK || (state[5*i+state_dim_base+2*WITH_TUTOR+3]==0 &&
						state[5*i+state_dim_base+2*WITH_TUTOR+4]==0)){
					blocks_under.push_back(i);
				}
			}
		}
		if (blocks_under.empty()) {
			int b = rng.uniformDiscrete(0, nbRedBlocks+nbBlueBlocks);
			res = (5+b);
		}
		else{
			if (state[0]==state[3] && state[1]==state[4]) {
				res = 3;
			}
		}
	}
	if (res==-1){
		res = 0;
	}
	return res;
}*/

int BlockRoom::trueBestAction(){
	int res = -1;
	if ((*block_hold) != -1) {
		if (((*agent_ns==*red_box_ns && *agent_ew==*red_box_ew)||
				(*agent_ns==*blue_box_ns && *agent_ew==*blue_box_ew)) && (*agent_ns==*agent_eye_ns && *agent_ew==*agent_eye_ew)){
			res = actions["PUT_IN"];
		}
		else if ((*agent_eye_ns != *red_box_ns || *agent_eye_ew!=*red_box_ew) && (*agent_eye_ns!=*blue_box_ns||*agent_eye_ew!=*blue_box_ew)){
			res = (rng.bernoulli(0.5) ? actions["LOOK_RED_BOX"] : actions["LOOK_BLUE_BOX"]);
		}
	}
	else if (*block_hold==-1){
		std::vector<int> blocks_under = find_block_under_eye();
		if (blocks_under.empty()) {
			int b;
			do {
				b = 0;
				if (nbRedBlocks+nbBlueBlocks-1 != 0){
					b = min(rng.uniformDiscrete(0, nbRedBlocks+nbBlueBlocks-1),nbRedBlocks+nbBlueBlocks-1);
				}
			}
			while(*(blocks[b].is_in_blue_box)||*(blocks[b].is_in_red_box));
			if (b<nbRedBlocks){
				res = actions["LOOK_RED_BLOCK_"+to_string(b)];
			}
			else{
				res = actions["LOOK_BLUE_BLOCK_"+to_string(b-nbRedBlocks)];
			}
		}
		else{
			if (*agent_ns==*agent_eye_ns && *agent_ew==*agent_eye_ew) {
				res = actions["PICK"];
			}
		}
	}
	if (res==-1){
		res = actions["GO_TO_EYE"];
	}
	return res;
}

tutor_feedback BlockRoom::tutorAction(){
	float tutor_reward = 0.;
	float reward = 0.;
	int tutoract;
	if (*block_hold!=-1 && *(blocks[*block_hold].color)==RED) {tutoract = tutor_actions["LOOK_RED_BOX"];}
	if (*block_hold!=-1 && *(blocks[*block_hold].color)==BLUE) {tutoract = tutor_actions["LOOK_BLUE_BOX"];}
	if (*block_hold==-1){
		bool found_block = false;
		int test = 0;
		while (!found_block && test<nbRedBlocks+nbBlueBlocks){
			found_block = *(blocks[test].is_in_blue_box)==0 && *(blocks[test].is_in_red_box)==0;
			test++;
		}
		if (found_block) {
			test--;
			std::string colorstring = *(blocks[test].color)==0 ? "RED" : "BLUE";
			int numblock = *(blocks[test].color)==0 ? test : test-nbRedBlocks;
			tutoract = tutor_actions["LOOK_"+colorstring+"_BLOCK_"+std::to_string(numblock)];
		}
		else{
			std::cout << "All blocks in boxes !" << std::endl;
		}

	}


	if (get_blocks_in()==nbRedBlocks+nbBlueBlocks){
		reward += 100;
		tutoract = tutor_actions["LOOK_BLUE_BLOCK_0"];
		tutor_reward += 100*(float)(get_blocks_right())/(float)(nbRedBlocks+nbBlueBlocks);
		reset();

//		if (get_blocks_right()==nbRedBlocks+nbBlueBlocks) {tutor_reward += 100;}
	}

	return tutor_feedback(tutor_reward, reward, tutoract);
}

void BlockRoom::tutorStop(){
	tutor_attentive = false;
}

