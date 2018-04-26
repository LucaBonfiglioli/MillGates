/*
 * State.cpp
 *
 *  Created on: Apr 21, 2018
 *      Author: Lorenzo Rosa (lorenzo.rosa.bo@gmail.com)
 */

#include "State.h"
#include "string.h"
#include "stdio.h"

//Constructors
State::State() {
	for (int i = 0; i < CUBE_SIZE_X; i++)
		for (int j = 0; j < CUBE_SIZE_Y; j++)
			for (int k = 0; k < CUBE_SIZE_Z; k++)
				pawns[i][j][k] = /*PAWN_NONE*/'O';

	phase = PHASE_1;
}

/*
7 O--------O--------O
6 |--O-----O-----O--|
5 |--|--O--O--O--|--|
4 O--O--O     O--O--O
3 |--|--O--O--O--|--|
2 |--O-----O-----O--|
1 O--------O--------O
  a  b  c  d  e  f  g
 */

State::State(std::string stringFromServer) : State() {
	/* Here, state is created (I call the void constructor)
	 * with all position void. Now, I only need to receive
	 * the positions and the content of all the cells
	 * in the format '<x><y><PAWN>'.
	 * Last three chars of the string indicates the number
	 * of pawns of each kind present on the board and the
	 * current phase.
	*/
	unsigned int i=0;

	//Pawns
	while(i<(stringFromServer.length()-3)) {
		setPawnAt(
			/* x */			stringFromServer[i],
			/* y */ 		stringFromServer[i+1],
			/* CONTENT */ 	stringFromServer[i+2]);
		i+=3;
	}

	// Three last chars
	setWhiteCheckersOnBoard(stringFromServer[i++]);
	setBlackCheckersOnBoard(stringFromServer[i++]);
	setPhase(stringFromServer[i]);
}

//Getters and setters
void State::setPawnAt(int x, int y, int z, pawn value) {
	pawns[x][y][z] = value;
}
pawn State::getPawnAt(int x, int y, int z) {
	return pawns[x][y][z];
}

bool State::setPawnAt(int8 x, int8 y, pawn value){
	int i, j, k;
	i = j = k = -1;

	//Convert from char to int
	y= y - '1' + 1;

	if ( x == 'd') { // 'd'
		//Coordinate 'x'
		j = 1;
		//Coordinate 'y'
		if ( y > 0 && y < 4) {
			i = 2;
			k =  3 - y;
		}
		else if ( y == 4) {
			i = 1;
			k = 1;
		}
		else if (y > 4 && y < 8) {
			i = 0;
			k = y % 5;
		}
		else return false;
	}
	else {
		//Coordinate 'x'
		if (x >= 'a' && (x/100) == 0) { // 'a','b','c'
		j = 0;
		k = 2 - (x % 'a');
		}
		else if (x <= 'g' && (x/100) == 1) { // 'e','f','g'
			j = 2;
			k = x % 'e';
		} else return false;

		//Coordinate 'y'
		if ( y > 0 && y < 4 )
			i = 2;
		else if ( y == 4)
			i = 1;
		else if (y > 4 && y < 8)
			i = 0;
		else return false;
	}

	if (i<0 || j<0 || k<0)
		return false;

	setPawnAt(i,j,k, value);
	return true;
}

// If coordinate is not valid, returns -1;
pawn State::getPawnAt(int8 x, int8 y){
	int i, j, k;
		i = j = k = -1;

		//Convert from char to int
		y= y - '1' + 1;

		if ( x == 'd') { // 'd'
			//Coordinate 'x'
			j = 1;
			//Coordinate 'y'
			if ( y > 0 && y < 4) {
				i = 2;
				k =  3 - y;
			}
			else if ( y == 4) {
				i = 1;
				k = 1;
			}
			else if (y > 4 && y < 8) {
				i = 0;
				k = y % 5;
			}
			else return -1;
		}
		else {
			//Coordinate 'x'
			if (x >= 'a' && (x/100) == 0) { // 'a','b','c'
			j = 0;
			k = 2 - (x % 'a');
			}
			else if (x <= 'g' && (x/100) == 1) { // 'e','f','g'
				j = 2;
				k = x % 'e';
			} else return -1;

			//Coordinate 'y'
			if ( y > 0 && y < 4 )
				i = 2;
			else if ( y == 4)
				i = 1;
			else if (y > 4 && y < 8)
				i = 0;
			else return -1;
		}

		if (i<0 || j<0 || k<0)
			return -1;

		return getPawnAt(i,j,k);
}

void State::setPhase(int8 currentPhase) {
	phase = currentPhase;
}
int8 State::getPhase() const {
	return phase;
}

void State::setWhiteCheckersOnBoard(int8 number){
	pawns[1][1][2] = number;
}
int8 State::getWhiteCheckersOnBoard() {
	return pawns[1][1][2];
}

void State::setBlackCheckersOnBoard(int8 number) {
	pawns[1][1][1] = number;
}
int8 State::getBlackCheckersOnBoard() {
	return pawns[1][1][1];
}

//Utiliy methods
State* State::clone() {

	State * res = new State();

	for (int i = 0; i < CUBE_SIZE_X; i++)
		for (int j = 0; j < CUBE_SIZE_Y; j++)
			for (int k = 0; k < CUBE_SIZE_Z; k++)

				res->setPawnAt(i, j, k, getPawnAt(i, j, k));

	res->setPhase(getPhase());

	return res;

}

std::string State::toString() const {

	static char res[CUBE_SIZE_X*CUBE_SIZE_Y*CUBE_SIZE_Z + 1];

	for (int i = 0; i < CUBE_SIZE_X; i++)
		for (int j = 0; j < CUBE_SIZE_Y; j++)
			for (int k = 0; k < CUBE_SIZE_Z; k++)

				res[i * CUBE_SIZE_X*CUBE_SIZE_Y + j * CUBE_SIZE_Z + k] = pawns[i][j][k]/*(pawns[i][j][k] == PAWN_NONE ? '0' : (pawns[i][j][k] == PAWN_BLACK ? 'B' : 'W'))*/;

	return std::string(res) + '('+ (char)phase+')';

}

int State::hash() {
	//TODO
	return 0;
}

State::~State() {}

std::ostream& operator<<(std::ostream &strm, const State &s){
	/* Here, I don't define the string representation directly,
	 * but I call the virtual method "toString". In this way, I
	 * emulate the Java behaviour of calling the "most specific"
	 * version of "toString" when printing an object (i.e., if the
	 * current instance is an instance of a subclass of State,
	 * I'll call the toString of that subclass).
	 */
	return strm << s.toString();
}


void State::toStringToSend(){
	char ret[25]; //TODO migliorare per inserire la casella al centro
				  //24 + \0

	//Insert left face
	int count = 0;
	int i, j=0, k;

	for(k=2; k>=0; k--)
		for(i=2; i>=0; i--){
			ret[count++] = pawns[i][j][k] /*== PAWN_WHITE ? 'W' : (pawns[i][j][k] == PAWN_NONE ? '0' : 'B')*/;
			printf("%c", ret[count - 1]);
		}

	//Insert middle line
	j=1;
	i=2;
	for(k=2; k>=0; k--){
		ret[count++] = pawns[i][j][k] /*== PAWN_WHITE ? 'W' : (pawns[i][j][k] == PAWN_NONE ? '0' : 'B')*/;
		printf("%c", ret[count - 1]);
	}
	i=0;
	for(k=0; k<=2; k++){
		ret[count++] = pawns[i][j][k] /*== PAWN_WHITE ? 'W' : (pawns[i][j][k] == PAWN_NONE ? '0' : 'B')*/;
		printf("%c", ret[count - 1]);
	}

	//Insert right face
	j=2;
	for(k=0; k<=2; k++)
		for(i=2; i>=0; i--){
			ret[count++] = pawns[i][j][k] /*== PAWN_WHITE ? 'W' : (pawns[i][j][k] == PAWN_NONE ? '0' : 'B')*/;
			printf("%c", ret[count - 1]);
		}


	ret[count] = '\0';
}

int main(int argc, char **argv) {
	printf("Program started...\n");

	//Create a state from string
	std::string str("a1Oa4Oa7Wb2Bb4Wb6Bc3Wc4Bc5Wd1Wd2Od3Od5Od6Od7Oe3Oe4Oe5Of2Wf4Wf6Bg1Og4Wg7O991"); //created string
	State s_from_string(str);
	std::cout << "State  created from string (internal repr): " << s_from_string << "\n";
	std::cout << "State  created from string (ordered repr): ";
	s_from_string.toStringToSend();
	std::cout << "\n";

	//Create a void state
	State s_void;
	std::cout << "State created void: " << s_void << "\n";

	//Here some tests
	s_void.setPawnAt('a', 1, 'W');
	s_void.setPawnAt('c', 5, 'B');
	s_void.setPawnAt('d', 4, 'W');
	s_void.setPhase(PHASE_3);

	std::cout << "State modified from setters (internal repr): " << s_void << "\n";
	std::cout << "State  created from string (ordered repr): ";
	s_void.toStringToSend();
	std::cout << "\n";
}


