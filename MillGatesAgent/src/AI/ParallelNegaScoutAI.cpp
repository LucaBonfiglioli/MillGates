/*
 * NegaScoutAI.cpp
 *
 *  Created on: May 11, 2018
 *      Author: Luca
 */

#include "ParallelNegaScoutAI.h"

ParallelNegaScoutAI::ParallelNegaScoutAI() {
	_tables = new ExpVector<HashSet<entry>*>(NUM_CORES);
	_histories = new ExpVector<HashSet<bool>*>(NUM_CORES);
	for (int i = 0; i < NUM_CORES; i++) {
		_tables->add(new HashSet<entry>());
		_histories->add(new HashSet<bool>());
	}
	_hasher = ZobristHashing::getInstance();
	_depth = MIN_SEARCH_DEPTH;
	_stopFlag = false;
	_heuristic = new RomanianHeuristic();
}

uint8 ParallelNegaScoutAI::getDepth() {
	return _depth;
}

void ParallelNegaScoutAI::setDepth(uint8 depth) {
	_depth = depth;
}

void ParallelNegaScoutAI::clear() {
	for (uint8 i = 0; i < NUM_CORES; i++)
		_tables->get(i)->clear();
}

void ParallelNegaScoutAI::stop() {
	_stopFlag = true;
}

void ParallelNegaScoutAI::addHistory(State * state) {

}

void ParallelNegaScoutAI::addHistory(State * state, int tid) {
	_histories->get(tid)->add(_hasher->hash(state), true);
}

void ParallelNegaScoutAI::clearHistory() {
	for (uint8 i = 0; i < NUM_CORES; i++) {
		_histories->get(i)->clear();
	}
}

void ParallelNegaScoutAI::setMaxFirst(ExpVector<State*> * states,
		ExpVector<hashcode> * hashes, ExpVector<eval_t> * values,
		ExpVector<Action> * actions) {
	// Find max
	eval_t max = values->get(0);
	eval_t indexMax = 0;
	for (eval_t i = 1; i < values->getLogicSize(); i++) {
		if (values->get(i) > max) {
			max = values->get(i);
			indexMax = i;
		}
	}
	if (indexMax != 0) {
		actions->swap(0, indexMax);
		states->swap(0, indexMax);
		hashes->swap(0, indexMax);
		values->swap(0, indexMax);
	}
}

eval_t ParallelNegaScoutAI::negaScout(State * state, hashcode quickhash,
	uint8 depth, eval_t alpha, eval_t beta, sint8 color, int tid) {

	entry * e;
	bool presentInTable = _tables->get(tid)->get(quickhash, &e);
	if (presentInTable && e->depth > depth) {
		if (e->entryFlag == EXACT)
			return color * e->eval;
		if (e->entryFlag == ALPHA_PRUNE && color * e->eval > alpha)
			alpha = e->eval * color;
		if (e->entryFlag == BETA_PRUNE && color * e->eval < beta)
			beta = e->eval * color;
		if (alpha >= beta)
			return alpha;
	}

	eval_t score = 0;
	bool terminal = state->isTerminal();
	bool loop = _histories->get(tid)->contains(quickhash) && depth != _depth + 1;

	if (depth == 0 || _stopFlag || loop || terminal) {
		score = ParallelNegaScoutAI::_heuristic->evaluate(state, terminal,
				loop);
		if (!presentInTable) {
			_tables->get(tid)->add(quickhash, entry { depth, EXACT, score });
			std::cout << "[" << tid << "] Added: " << quickhash << "\n";
		}
		else
			*e = {depth, EXACT, score};
		return color * score;
	}

	ExpVector<Action> * actions = state->getActions();
	ExpVector<State*> * states = new ExpVector<State*>(actions->getLogicSize());
	ExpVector<hashcode> * hashes = new ExpVector<hashcode>(
			actions->getLogicSize());
	ExpVector<eval_t> * values = new ExpVector<eval_t>(actions->getLogicSize());

	entry * e_tmp;

	bool child_loop, child_present;

	for (eval_t i = 0; i < actions->getLogicSize(); i++) {

		states->add(state->result(actions->get(i)));
		hashes->add(_hasher->quickHash(state, actions->get(i), quickhash));
		child_present = _tables->get(tid)->get(hashes->get(i), &e_tmp);

		if (child_present && e_tmp->depth > depth - 1)
			values->add(e_tmp->eval * -color);
		else { //Else I have to estimate the value using function
			child_loop = _histories->get(tid)->contains(hashes->get(i));
			values->add(
					_heuristic->evaluate(states->get(i),
							states->get(i)->isTerminal(), child_loop) * -color);
		}

	}

	setMaxFirst(states, hashes, values, actions);

	State * child = NULL;
	hashcode child_hash = 0;
	entryFlag_t flag = ALPHA_PRUNE;
	for (eval_t i = 0; i < actions->getLogicSize(); i++) {

		child = states->get(i);
		child_hash = hashes->get(i);
		if (i == 0)
			score = -negaScout(child, child_hash, depth - 1, -beta, -alpha,
					-color, tid);
		else {
			score = -negaScout(child, child_hash, depth - 1, -alpha - 1, -alpha,
					-color, tid);
			if (score > alpha && score < beta)
				score = -negaScout(child, child_hash, depth - 1, -beta, -score,
						-color, tid);
		}
		delete child;
		states->set(i, NULL);

		if (score > alpha) {
			alpha = score;
			flag = EXACT;
		}

		if (alpha >= beta) {
			flag = BETA_PRUNE;
			break;
		}
	}
	delete actions;
	delete hashes;
	for (uint8 i = 0; i < states->getLogicSize(); i++)
		if (states->get(i) != NULL)
			delete states->get(i);
	delete states;
	delete values;

	if (!presentInTable)
		_tables->get(tid)->add(quickhash, entry { depth, flag, (eval_t) (alpha * color) });
	else
		*e = {depth, flag, (eval_t)(alpha * color)};

	return alpha;

}

void * ParallelNegaScoutAI::negaScoutThread_helper(void * param) {
	return (reinterpret_cast<ParallelNegaScoutAI*>(((void**) param)[0]))->negaScoutThread(
			reinterpret_cast<args*>(((void**) param)[1]));
}

void * ParallelNegaScoutAI::negaScoutThread(args * arg) {

	// TODO: debug
	std::cout << "Ciao, sono il thread " << (int)arg->tid << "\n";
//	for(int i = 0; i < arg->actions->getLogicSize(); i++) {
//		std::cout << arg->actions->get(i) << "\n";
//	}
	//TODO: debug

	//Check the table??

	//THIS PART EQUALS TO NEGASCOUT
	HashSet<entry> * _table = _tables->get(arg->tid);

	ExpVector<State*> * states = new ExpVector<State*>(arg->actions->getLogicSize());
	ExpVector<hashcode> * hashes = new ExpVector<hashcode>(arg->actions->getLogicSize());
	ExpVector<eval_t> * values = new ExpVector<eval_t>(arg->actions->getLogicSize());


	entry * e_tmp;
	bool child_loop, child_present;

	for (int i = 0; i < arg->actions->getLogicSize(); i++) {
		states->add(arg->state->result(arg->actions->get(i)));
		hashes->add(
				_hasher->quickHash(arg->state, arg->actions->get(i),
						arg->hash));

		child_present = _table->get(hashes->get(i), &e_tmp);
		if (child_present && e_tmp->depth > arg->depth - 1)
			values->add(e_tmp->eval * -arg->color);
		else { //Else I have to estimate the value using function
			child_loop = _histories->get(arg->tid)->contains(hashes->get(i));
			values->add(
					_heuristic->evaluate(states->get(i),
							states->get(i)->isTerminal(), child_loop)
							* -arg->color);
		}
	}
	setMaxFirst(states, hashes, values, arg->actions);
	std::cout << "MAX: " << arg->actions->get(0) << "\n";

	State * child = NULL;
	hashcode child_hash = 0;
	entryFlag_t flag = ALPHA_PRUNE;
	int alpha = -MAX_EVAL_T;
	int beta = MAX_EVAL_T;
	eval_t score = 0;
	for (int i = 0; i < arg->actions->getLogicSize(); i++) {
		child = states->get(i);
		child_hash = hashes->get(i);

		//DEBUG
		if (i == 0)
			score = -negaScout(child, child_hash, arg->depth - 1, -beta, -alpha,
					-arg->color, arg->tid);
		else {
			score = -negaScout(child, child_hash, arg->depth - 1, -alpha - 1,
					-alpha, -arg->color, arg->tid);
			if (score > alpha && score < beta)
				score = -negaScout(child, child_hash, arg->depth - 1, -beta,
						-score, -arg->color, arg->tid);
		}
		delete child;
		states->set(i, NULL);

		if (score > alpha) {
			alpha = score;
			flag = EXACT;
		}

		if (alpha >= beta) {
			flag = BETA_PRUNE;
			break;
		}
	}
	delete hashes;
	for (uint8 i = 0; i < states->getLogicSize(); i++)
		if (states->get(i) != NULL)
			delete states->get(i);
	delete states;
	delete values;

//		if (!presentInTable)
//			_table->add(quickhash, entry { depth, flag, (eval_t) (alpha * color) })
//			;
//		else
//			*e = {depth, flag, (eval_t)(alpha * color)};
	// EXPLODES! WHY?
	//	arg->actions = NULL;
	//	delete arg->actions;
//	arg = NULL;
//	delete arg;
	pthread_exit(NULL);
	return NULL;
}

Action ParallelNegaScoutAI::choose(State * state) {

	_stopFlag = false;

	ExpVector<Action> * actions = state->getActions();

	Action res;
	entry * tempscore;
	hashcode quickhash, hash;

	hash = _hasher->hash(state);
	sint8 color = (state->getPlayer() == PAWN_WHITE) ? 1 : -1;
	eval_t score = MAX_EVAL_T * -color;

	// Thread creation
	void ** param = (void**) malloc(sizeof(void*) * 2);
	param[0] = this;

	pthread_t thread[NUM_CORES];
	int rc;
	args * arguments;
	ExpVector<pthread_t> threads;

	//create the action to give to each thread
	ExpVector<Action> * actions_for_thread;
	int num_actions_per_thread = (actions->getLogicSize() / NUM_CORES);
	int rest_of_actions = (actions->getLogicSize() % NUM_CORES);
	for (int i = 0; i < NUM_CORES; i++) {
		actions_for_thread = new ExpVector<Action>();
		for (int k = i * num_actions_per_thread;
				k < (((pthread_t)i * num_actions_per_thread) + num_actions_per_thread
								+ ((i == NUM_CORES - 1) ? rest_of_actions : 0));
				k++)
			actions_for_thread->add(actions->get(k));

		arguments =
				new args {i, state, actions_for_thread, hash, color, _depth };
		param[1] = arguments;
		rc = pthread_create(&thread[i], NULL, negaScoutThread_helper,
				param);
		std::cout << "Avviato thread: " << thread[i] << "\n";
		if (rc) {
			std::cout << "ERRORE: " << rc;
			exit(-1);
		}
		actions_for_thread = NULL;
	}

	for (int t = 0; t < NUM_CORES; t++) {
		rc = pthread_join(thread[t], NULL);
	}
	clear();
	delete actions_for_thread;
	free(param);

	for(int j=0; j<NUM_CORES; j++) {
		for (int i = 0; i < actions->getLogicSize(); i++) {
			quickhash = _hasher->quickHash(state, actions->get(i), hash);
			_tables->get(j)->get(quickhash, &tempscore);
			if (tempscore != NULL && color == 1) {
				if (tempscore->eval > score && tempscore->entryFlag == EXACT) {
					score = tempscore->eval;
					std::cout << "[" << j << "] Score (" << i << "): " << score << "\n";
					res = actions->get(i);
				}
			} else if (tempscore != NULL && color == -1) {
				if (tempscore->eval < score && tempscore->entryFlag == EXACT) {
					score = tempscore->eval;
					std::cout << "[" << j << "] Score (" << i << "): " << score << "\n";
					res = actions->get(i);
				}

			}
		}
		std::cout << "Action chosen by " << j << ": " << res << "\n";
	}

	/*
	 * DEBUG!!!!
	 */
//	res = actions->get(0);

	delete actions;
	return res;
}

void ParallelNegaScoutAI::recurprint(State * state, int depth, int curdepth) {
	entry * val = NULL;
	ExpVector<Action> * actions = state->getActions();
	State * child = NULL;
	for (int j = 0; j < NUM_CORES; j++) {
		std::cout << "TABLE OF THREAD " << j << "\n ==== \n";
		for (int i = 0; i < actions->getLogicSize(); i++) {
			child = state->result(actions->get(i));
			_tables->get(j)->get(_hasher->hash(child), &val);
			if (val == NULL) {
				for (int j = 0; j < curdepth; j++)
					std::cout << " | ";
				std::cout << actions->get(i) << " -> CUT \n";
			} else if (depth == curdepth) {
				for (int j = 0; j < curdepth; j++)
					std::cout << " | ";
				std::cout << actions->get(i) << " -> " << (int) val->eval
						<< ", " << (int) val->entryFlag << ", "
						<< (int) val->depth << "\n";
			} else {
				for (int j = 0; j < curdepth; j++)
					std::cout << " | ";
				std::cout << actions->get(i) << " { \n";
				recurprint(child, depth, curdepth + 1);
				for (int j = 0; j < curdepth; j++)
					std::cout << " | ";
				std::cout << "} -> " << (int) val->eval << ", "
						<< (int) val->entryFlag << ", " << (int) val->depth
						<< "\n";
			}
		}
		delete child;
	}
}

void ParallelNegaScoutAI::print(State * state, int depth) {
	recurprint(state, depth, 0);
}

ParallelNegaScoutAI::~ParallelNegaScoutAI() {
	for (int i = 0; i < NUM_CORES; i++) {
		delete _tables->get(i);
		delete _histories->get(i);
	}
	delete _tables;
	delete _histories;
	delete _heuristic;
}