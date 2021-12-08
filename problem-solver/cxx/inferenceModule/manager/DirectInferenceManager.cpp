/*
* This source file is part of an OSTIS project. For the latest info, see http://ostis.net
* Distributed under the MIT License
* (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
*/

#include "DirectInferenceManager.hpp"

#include <sc-memory/sc_addr.hpp>
#include <sc-agents-common/utils/GenerationUtils.hpp>
#include <sc-agents-common/utils/IteratorUtils.hpp>
#include <sc-agents-common/utils/LogicRuleUtils.hpp>
#include <logic/LogicExpressionChecker.h>

#include "utils/ContainersUtils.hpp"

using namespace inference;
using namespace utils;

DirectInferenceManager::DirectInferenceManager(ScMemoryContext * ms_context) : ms_context(ms_context)
{
  this->solutionTreeManager = new SolutionTreeGenerator(ms_context);
  this->templateManager = new TemplateManager(ms_context);
  this->templateSearcher = new TemplateSearcher(ms_context);
}

DirectInferenceManager::~DirectInferenceManager()
{
  delete this->solutionTreeManager;
  delete this->templateManager;
  delete this->templateSearcher;
}

ScAddr DirectInferenceManager::applyInference(
      const ScAddr & targetStatement,
      const ScAddr & ruleSet,
      const ScAddr & argumentSet)
{
  queue<ScAddr> uncheckedRules = createQueue(ruleSet);
  vector<ScAddr> argumentList = IteratorUtils::getAllWithType(ms_context, argumentSet, ScType::Node);
  vector<ScAddr> checkedRuleList;
SC_LOG_INFO("1")
  bool targetAchieved = isTargetAchieved(targetStatement, argumentList);
    SC_LOG_INFO("2")
  ScAddr rule;
  bool isUsed;
  if (!targetAchieved)
  {
      SC_LOG_INFO("4")
    while (!uncheckedRules.empty())
    {
        SC_LOG_INFO("5")
      rule = uncheckedRules.front();
        SC_LOG_INFO("6")
      isUsed = useRule(rule, argumentList);
        SC_LOG_INFO("7")
      if (isUsed)
      {
        targetAchieved = isTargetAchieved(targetStatement, argumentList);
        if (targetAchieved)
        {
          SC_LOG_DEBUG("Target achieved");
          break;
        }
        else
        {
          ContainersUtils::addToQueue(checkedRuleList, uncheckedRules);
          checkedRuleList.clear();
        }
      }
      else
      {
        checkedRuleList.push_back(rule);
      }
      uncheckedRules.pop();
    }
  }
  else
  { SC_LOG_DEBUG("Target is already achieved") }

  return this->solutionTreeManager->createSolution(targetAchieved);
}

queue<ScAddr> DirectInferenceManager::createQueue(ScAddr const & set)
{
  queue<ScAddr> queue;
  vector<ScAddr> elementList = IteratorUtils::getAllWithType(ms_context, set, ScType::Node);

  ContainersUtils::addToQueue(elementList, queue);
  return queue;
}

bool DirectInferenceManager::useRule(ScAddr const & rule, vector<ScAddr> const & argumentList)
{
  SC_LOG_INFO("Trying to use rule: " + ms_context->HelperGetSystemIdtf(rule));
  bool isUsed = false;
    SC_LOG_INFO(ms_context->HelperGetSystemIdtf(rule))
  ScAddr ifStatement = LogicRuleUtils::getIfStatement(ms_context, rule);

SC_LOG_INFO(ms_context->HelperGetSystemIdtf(ifStatement))
  //collect all the templates within if condition
  //generate for them sets of ScTemplateParams
  //choose the set when IsEmpty() is false, memorize the template the set was generated for
  LogicExpression logicExpression(ifStatement, ms_context,
          templateSearcher, templateManager, argumentList);
  logicExpression.build();
    SC_LOG_INFO("Expression is built");
  SC_LOG_INFO(logicExpression.toString());

  //vector<ScTemplateParams> ifStatementParamsList = templateManager->createTemplateParamsList(ifStatement, argumentList);
  SC_LOG_INFO("Created " + to_string(logicExpression.GetParamsSet().size()) + " statement params variants");

  //use that set
  for (const auto& ifStatementParams : logicExpression.GetParamsSet())
  {
    //replace on logic checker
    //return search result of the memorized template
/*    vector<ScTemplateSearchResultItem> searchResult =
        templateSearcher->searchTemplate(ifStatement, ifStatementParams);*/
    auto result = logicExpression.GetRoot().check(
              logicExpression.GetReferenceTemplate(), ifStatementParams);
    bool success = std::get<0>(result);
    ScTemplateSearchResultItem resultItem = std::get<2>(result);
    SC_LOG_INFO("Success?" + std::to_string(success));
    if (success)
    {
      ScAddr elseStatement = LogicRuleUtils::getElseStatement(ms_context, rule);
    SC_LOG_INFO(ms_context->HelperGetSystemIdtf(elseStatement));
    SC_LOG_INFO(std::to_string(resultItem.Size()));
      ScTemplateParams elseStatementParams;
      if (resultItem.Size() > 0)
      {
          SC_LOG_INFO("res_item");
        vector<ScAddr> varList = IteratorUtils::getAllWithType(ms_context, elseStatement, ScType::NodeVar);
        for (auto var : varList)
        {
            SC_LOG_INFO("var: " + ms_context->HelperGetSystemIdtf(var));
          if (ms_context->HelperCheckEdge(std::get<3>(result), var, ScType::EdgeAccessConstPosPerm))
          {
              SC_LOG_INFO("var_checked: " + ms_context->HelperGetSystemIdtf(var));
            string varName = ms_context->HelperGetSystemIdtf(var);
            ScAddr node;
            ifStatementParams.Get(varName, node);
            if (!node.IsValid())
            {
              node = resultItem[varName];
            }
            elseStatementParams.Add(varName, node);
          }
        }
      }
      if (generateStatement(elseStatement, elseStatementParams))
      {
        this->solutionTreeManager->addNode(rule, ifStatementParams);
        isUsed = true;
        SC_LOG_DEBUG("Rule used");
      }
    }
  }
  return isUsed;
}

bool DirectInferenceManager::generateStatement(ScAddr const & statement, ScTemplateParams const & templateParams)
{
  bool result = false;
  ScTemplate searchTemplate;
  ms_context->HelperBuildTemplate(searchTemplate, statement, templateParams);
  ScTemplateSearchResult templateSearchResult;
  if (!ms_context->HelperSearchTemplate(searchTemplate, templateSearchResult))
  {
    ScTemplate statementTemplate;
    ms_context->HelperBuildTemplate(statementTemplate, statement);
    ScTemplateGenResult templateGenResult;
    result = ms_context->HelperGenTemplate(statementTemplate, templateGenResult, templateParams);
  }
  return result;
}

bool DirectInferenceManager::isTargetAchieved(ScAddr const & targetStatement, vector<ScAddr> const & argumentList)
{
  bool result = false;
  ScTemplateParams templateParams = templateManager->createTemplateParams(targetStatement, argumentList);
    SC_LOG_INFO("8")
  vector<ScTemplateSearchResultItem> searchResult =
      templateSearcher->searchTemplate(targetStatement, templateParams);
  if (!searchResult.empty())
  { result = true; }
  return result;
}
