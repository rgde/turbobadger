// ================================================================================
// == This file is a part of Tinkerbell UI Toolkit. (C) 2011-2012, Emil Seger�s ==
// ==                   See tinkerbell.h for more information.                   ==
// ================================================================================

#include "tb_test.h"
#include "tb_system.h"

namespace tinkerbell {

#ifdef TB_UNIT_TESTING

uint32 test_settings;
int fail_line_nr;
const char *fail_file;
const char *fail_text;

TBLinkListOf<TBTestGroup> groups;

// == Misc functions ==========================================================

TBStr tb_get_test_file_name(const char *testpath, const char *filename)
{
	TBStr str;
	int test_path_len = strlen(testpath);
	for (int i = test_path_len - 1; i > 0 && testpath[i] != '/' && testpath[i] != '\\'; i--)
		test_path_len = i;
	str.Set(testpath, test_path_len);
	str.Append(filename);
	return str;
}

// == TBRegisterCall ==========================================================

TBRegisterCall::TBRegisterCall(TBTestGroup *test, TBCall *call)
	: call(call)
{
	if (strcmp(call->name(), "Setup") == 0)
		test->setup = call;
	else if (strcmp(call->name(), "Cleanup") == 0)
		test->cleanup = call;
	else if (strcmp(call->name(), "Init") == 0)
		test->init = call;
	else if (strcmp(call->name(), "Shutdown") == 0)
		test->shutdown = call;
	else
		test->calls.AddLast(call);
}

TBRegisterCall::~TBRegisterCall()
{
	if (call->linklist)
		call->linklist->Remove(call);
}

// == TBTestGroup =============================================================

TBTestGroup::TBTestGroup(const char *name)
	: name(name), setup(nullptr), cleanup(nullptr), init(nullptr), shutdown(nullptr)
{
	groups.AddLast(this);
}

TBTestGroup::~TBTestGroup()
{
	groups.Remove(this);
}

const char *CallAndOutput(TBTestGroup *test, TBCall *call)
{
	fail_text = nullptr;
	call->exec();

	if (!fail_text)
		return fail_text;
	TBStr msg;
	msg.SetFormatted("FAIL: \"%s/%s\":\n"
					"  %s(%d): \"%s\"\n",
					test->name, call->name(),
					fail_file, fail_line_nr, fail_text);
	TBDebugOut(msg);
	return fail_text;
}

void OutputPass(TBTestGroup *test, const char *call_name)
{
	if (!(test_settings & TB_TEST_VERBOSE))
		return;
	TBStr msg;
	msg.SetFormatted("PASS: \"%s/%s\"\n",
					test->name, call_name);
	TBDebugOut(msg);
}

int TBRunTests(uint32 settings)
{
	test_settings = settings;
	int num_failed = 0;
	int num_passed = 0;

	TBDebugOut("Running tests...\n");

	TBLinkListOf<TBTestGroup>::Iterator i = groups.IterateForward();
	while (TBTestGroup *group = i.GetAndStep())
	{
		if (group->init && CallAndOutput(group, group->init))
		{
			// The whole group failed because init failed.
			int num_tests_in_group = 0;
			for (TBCall *call = group->calls.GetFirst(); call; call = call->GetNext())
				if (!group->IsSpecialTest(call))
					num_tests_in_group++;

			TBStr msg;
			msg.SetFormatted("  %d tests skipped.\n", num_tests_in_group);
			TBDebugOut(msg);

			num_failed += num_tests_in_group;
			continue;
		}
		for (TBCall *call = group->calls.GetFirst(); call; call = call->GetNext())
		{
			// Execute test (and call setup and cleanup if available).
			int fail = 0;
			if (group->setup)
				fail = !!CallAndOutput(group, group->setup);
			if (!fail) // Only run if setup succeeded
			{
				fail |= !!CallAndOutput(group, call);
				if (group->cleanup)
					fail |= !!CallAndOutput(group, group->cleanup);
			}
			// Handle result
			if (fail)
				num_failed++;
			else
			{
				num_passed++;
				OutputPass(group, call->name());
			}
		}
		if (group->shutdown && CallAndOutput(group, group->shutdown))
			CallAndOutput(group, group->shutdown);
	}

	TBStr msg;
	msg.SetFormatted("Test results: %d passed, %d failed.\n", num_passed, num_failed);
	TBDebugOut(msg);
	return num_failed;
}

#endif // TB_UNIT_TESTING

}; // namespace tinkerbell