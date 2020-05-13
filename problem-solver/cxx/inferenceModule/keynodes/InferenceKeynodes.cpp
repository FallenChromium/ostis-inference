/*
* This source file is part of an OSTIS project. For the latest info, see http://ostis.net
* Distributed under the MIT License
* (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
*/

#include <sc-memory/cpp/sc_memory.hpp>

#include "InferenceKeynodes.hpp"

namespace inference
{

ScAddr InferenceKeynodes::question_use_logic_rule;
ScAddr InferenceKeynodes::question_reverse_inference;
ScAddr InferenceKeynodes::question_direct_inference;
ScAddr InferenceKeynodes::solution;
ScAddr InferenceKeynodes::success_solution;
ScAddr InferenceKeynodes::rrel_solver_key_sc_element;

}
