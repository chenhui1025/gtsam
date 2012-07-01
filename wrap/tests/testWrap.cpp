/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file testWrap.cpp
 * @brief Unit test for wrap.c
 * @author Frank Dellaert
 **/

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/assign/std/vector.hpp>
#include <boost/filesystem.hpp>
#include <CppUnitLite/TestHarness.h>

#include <wrap/utilities.h>
#include <wrap/Module.h>

using namespace std;
using namespace boost::assign;
using namespace wrap;
namespace fs = boost::filesystem;
static bool enable_verbose = false;
#ifdef TOPSRCDIR
static string topdir = TOPSRCDIR;
#else
static string topdir = "TOPSRCDIR_NOT_CONFIGURED"; // If TOPSRCDIR is not defined, we error
#endif

typedef vector<string> strvec;

// NOTE: as this path is only used to generate makefiles, it is hardcoded here for testing
// In practice, this path will be an absolute system path, which makes testing it more annoying
static const std::string headerPath = "/not_really_a_real_path/borg/gtsam/wrap";

/* ************************************************************************* */
TEST( wrap, ArgumentList ) {
	ArgumentList args;
	Argument arg1; arg1.type = "double"; arg1.name = "x";
	Argument arg2; arg2.type = "double"; arg2.name = "y";
	Argument arg3; arg3.type = "double"; arg3.name = "z";
	args.push_back(arg1);
	args.push_back(arg2);
	args.push_back(arg3);
	EXPECT(assert_equal("ddd", args.signature()));
	EXPECT(assert_equal("double,double,double", args.types()));
	EXPECT(assert_equal("x,y,z", args.names()));
}

/* ************************************************************************* */
TEST( wrap, check_exception ) {
	THROWS_EXCEPTION(Module("/notarealpath", "geometry",enable_verbose));
	CHECK_EXCEPTION(Module("/alsonotarealpath", "geometry",enable_verbose), CantOpenFile);

	// clean out previous generated code
  fs::remove_all("actual_deps");

	string path = topdir + "/wrap/tests";
	Module module(path.c_str(), "testDependencies",enable_verbose);
	CHECK_EXCEPTION(module.matlab_code("mex", "actual_deps", "mexa64", headerPath, "-O5"), DependencyMissing);
}

/* ************************************************************************* */
TEST( wrap, parse ) {
	string markup_header_path = topdir + "/wrap/tests";
	Module module(markup_header_path.c_str(), "geometry",enable_verbose);
	EXPECT_LONGS_EQUAL(3, module.classes.size());

	// check using declarations
	strvec exp_using1, exp_using2; exp_using2 += "geometry";

	// forward declarations
	strvec exp_forward; exp_forward += "VectorNotEigen", "ns::OtherClass";
	EXPECT(assert_equal(exp_forward, module.forward_declarations));

	// check first class, Point2
	{
		Class cls = module.classes.at(0);
		EXPECT(assert_equal("Point2", cls.name));
		EXPECT_LONGS_EQUAL(2, cls.constructor.args_list.size());
		EXPECT_LONGS_EQUAL(7, cls.methods.size());
		EXPECT_LONGS_EQUAL(0, cls.static_methods.size());
		EXPECT_LONGS_EQUAL(0, cls.namespaces.size());
		EXPECT(assert_equal(exp_using1, cls.using_namespaces));
	}

	// check second class, Point3
	{
		Class cls = module.classes.at(1);
		EXPECT(assert_equal("Point3", cls.name));
		EXPECT_LONGS_EQUAL(1, cls.constructor.args_list.size());
		EXPECT_LONGS_EQUAL(1, cls.methods.size());
		EXPECT_LONGS_EQUAL(2, cls.static_methods.size());
		EXPECT_LONGS_EQUAL(0, cls.namespaces.size());
		EXPECT(assert_equal(exp_using2, cls.using_namespaces));

		// first constructor takes 3 doubles
		ArgumentList c1 = cls.constructor.args_list.front();
		EXPECT_LONGS_EQUAL(3, c1.size());

		// check first double argument
		Argument a1 = c1.front();
		EXPECT(!a1.is_const);
		EXPECT(assert_equal("double", a1.type));
		EXPECT(!a1.is_ref);
		EXPECT(assert_equal("x", a1.name));

		// check method
		Method m1 = cls.methods.front();
		EXPECT(assert_equal("double", m1.returnVal.type1));
		EXPECT(assert_equal("norm", m1.name));
		EXPECT_LONGS_EQUAL(0, m1.args.size());
		EXPECT(m1.is_const_);
	}

	// Test class is the third one
	{
		LONGS_EQUAL(3, module.classes.size());
		Class testCls = module.classes.at(2);
		EXPECT_LONGS_EQUAL( 2, testCls.constructor.args_list.size());
		EXPECT_LONGS_EQUAL(19, testCls.methods.size());
		EXPECT_LONGS_EQUAL( 0, testCls.static_methods.size());
		EXPECT_LONGS_EQUAL( 0, testCls.namespaces.size());
		EXPECT(assert_equal(exp_using2, testCls.using_namespaces));
		strvec exp_includes; exp_includes += "folder/path/to/Test.h";
		EXPECT(assert_equal(exp_includes, testCls.includes));

		// function to parse: pair<Vector,Matrix> return_pair (Vector v, Matrix A) const;
		Method m2 = testCls.methods.front();
		EXPECT(m2.returnVal.isPair);
		EXPECT(m2.returnVal.category1 == ReturnValue::EIGEN);
		EXPECT(m2.returnVal.category2 == ReturnValue::EIGEN);
	}
}

/* ************************************************************************* */
TEST( wrap, parse_namespaces ) {
	string header_path = topdir + "/wrap/tests";
	Module module(header_path.c_str(), "testNamespaces",enable_verbose);
	EXPECT_LONGS_EQUAL(6, module.classes.size());

	{
		Class cls = module.classes.at(0);
		EXPECT(assert_equal("ClassA", cls.name));
		strvec exp_namespaces; exp_namespaces += "ns1";
		EXPECT(assert_equal(exp_namespaces, cls.namespaces));
		strvec exp_includes; exp_includes += "path/to/ns1.h", "";
		EXPECT(assert_equal(exp_includes, cls.includes));
	}

	{
		Class cls = module.classes.at(1);
		EXPECT(assert_equal("ClassB", cls.name));
		strvec exp_namespaces; exp_namespaces += "ns1";
		EXPECT(assert_equal(exp_namespaces, cls.namespaces));
		strvec exp_includes; exp_includes += "path/to/ns1.h", "path/to/ns1/ClassB.h";
		EXPECT(assert_equal(exp_includes, cls.includes));
	}

	{
		Class cls = module.classes.at(2);
		EXPECT(assert_equal("ClassA", cls.name));
		strvec exp_namespaces; exp_namespaces += "ns2";
		EXPECT(assert_equal(exp_namespaces, cls.namespaces));
		strvec exp_includes; exp_includes += "path/to/ns2.h", "path/to/ns2/ClassA.h";
		EXPECT(assert_equal(exp_includes, cls.includes));
	}

	{
		Class cls = module.classes.at(3);
		EXPECT(assert_equal("ClassB", cls.name));
		strvec exp_namespaces; exp_namespaces += "ns2", "ns3";
		EXPECT(assert_equal(exp_namespaces, cls.namespaces));
		strvec exp_includes; exp_includes += "path/to/ns2.h", "path/to/ns3.h", "";
		EXPECT(assert_equal(exp_includes, cls.includes));
	}

	{
		Class cls = module.classes.at(4);
		EXPECT(assert_equal("ClassC", cls.name));
		strvec exp_namespaces; exp_namespaces += "ns2";
		EXPECT(assert_equal(exp_namespaces, cls.namespaces));
		strvec exp_includes; exp_includes += "path/to/ns2.h", "";
		EXPECT(assert_equal(exp_includes, cls.includes));
	}

	{
		Class cls = module.classes.at(5);
		EXPECT(assert_equal("ClassD", cls.name));
		strvec exp_namespaces;
		EXPECT(assert_equal(exp_namespaces, cls.namespaces));
		strvec exp_includes; exp_includes += "";
		EXPECT(assert_equal(exp_includes, cls.includes));
	}

}

/* ************************************************************************* */
TEST( wrap, matlab_code_namespaces ) {
	string header_path = topdir + "/wrap/tests";
	Module module(header_path.c_str(), "testNamespaces",enable_verbose);
	EXPECT_LONGS_EQUAL(6, module.classes.size());
	string path = topdir + "/wrap";

	// clean out previous generated code
  fs::remove_all("actual_namespaces");

	// emit MATLAB code
  string exp_path = path + "/tests/expected_namespaces/";
  string act_path = "actual_namespaces/";
	module.matlab_code("mex", "actual_namespaces", "mexa64", headerPath, "-O5");

	EXPECT(files_equal(exp_path + "new_ClassD_.cpp"              , act_path + "new_ClassD_.cpp"              ));
	EXPECT(files_equal(exp_path + "new_ClassD_.m"                , act_path + "new_ClassD_.m"                ));
	EXPECT(files_equal(exp_path + "new_ns1ClassA_.cpp"           , act_path + "new_ns1ClassA_.cpp"           ));
	EXPECT(files_equal(exp_path + "new_ns1ClassA_.m"             , act_path + "new_ns1ClassA_.m"             ));
	EXPECT(files_equal(exp_path + "new_ns1ClassB_.cpp"           , act_path + "new_ns1ClassB_.cpp"           ));
	EXPECT(files_equal(exp_path + "new_ns1ClassB_.m"             , act_path + "new_ns1ClassB_.m"             ));
	EXPECT(files_equal(exp_path + "new_ns2ClassA_.cpp"           , act_path + "new_ns2ClassA_.cpp"           ));
	EXPECT(files_equal(exp_path + "new_ns2ClassA_.m"             , act_path + "new_ns2ClassA_.m"             ));
	EXPECT(files_equal(exp_path + "new_ns2ClassC_.cpp"           , act_path + "new_ns2ClassC_.cpp"           ));
	EXPECT(files_equal(exp_path + "new_ns2ClassC_.m"             , act_path + "new_ns2ClassC_.m"             ));
	EXPECT(files_equal(exp_path + "new_ns2ns3ClassB_.cpp"        , act_path + "new_ns2ns3ClassB_.cpp"        ));
	EXPECT(files_equal(exp_path + "new_ns2ns3ClassB_.m"          , act_path + "new_ns2ns3ClassB_.m"          ));
	EXPECT(files_equal(exp_path + "ns2ClassA_afunction.cpp"      , act_path + "ns2ClassA_afunction.cpp"      ));
	EXPECT(files_equal(exp_path + "ns2ClassA_afunction.m"        , act_path + "ns2ClassA_afunction.m"        ));

	EXPECT(files_equal(exp_path + "@ns2ClassA/memberFunction.cpp", act_path + "@ns2ClassA/memberFunction.cpp"));
	EXPECT(files_equal(exp_path + "@ns2ClassA/memberFunction.m"  , act_path + "@ns2ClassA/memberFunction.m"  ));
	EXPECT(files_equal(exp_path + "@ns2ClassA/ns2ClassA.m"       , act_path + "@ns2ClassA/ns2ClassA.m"       ));
	EXPECT(files_equal(exp_path + "@ns2ClassA/nsArg.cpp"         , act_path + "@ns2ClassA/nsArg.cpp"         ));
	EXPECT(files_equal(exp_path + "@ns2ClassA/nsArg.m"           , act_path + "@ns2ClassA/nsArg.m"           ));
	EXPECT(files_equal(exp_path + "@ns2ClassA/nsReturn.cpp"      , act_path + "@ns2ClassA/nsReturn.cpp"      ));
	EXPECT(files_equal(exp_path + "@ns2ClassA/nsReturn.m"        , act_path + "@ns2ClassA/nsReturn.m"        ));

	EXPECT(files_equal(exp_path + "make_testNamespaces.m", act_path + "make_testNamespaces.m"));
	EXPECT(files_equal(exp_path + "Makefile"       , act_path + "Makefile"       ));
}

/* ************************************************************************* */
TEST( wrap, matlab_code ) {
	// Parse into class object
	string header_path = topdir + "/wrap/tests";
	Module module(header_path,"geometry",enable_verbose);
	string path = topdir + "/wrap";

	// clean out previous generated code
  fs::remove_all("actual");

	// emit MATLAB code
	// make_geometry will not compile, use make testwrap to generate real make
	module.matlab_code("mex", "actual", "mexa64", headerPath, "-O5");
  string epath = path + "/tests/expected/";
  string apath = "actual/";

	EXPECT(files_equal(epath + "Makefile"                     , apath + "Makefile"                     ));
	EXPECT(files_equal(epath + "make_geometry.m"              , apath + "make_geometry.m"              ));
	EXPECT(files_equal(epath + "new_Point2_.cpp"              , apath + "new_Point2_.cpp"              ));
	EXPECT(files_equal(epath + "new_Point2_.m"                , apath + "new_Point2_.m"                ));
	EXPECT(files_equal(epath + "new_Point3_.cpp"              , apath + "new_Point3_.cpp"              ));
	EXPECT(files_equal(epath + "new_Point3_.m"                , apath + "new_Point3_.m"                ));
	EXPECT(files_equal(epath + "new_Test_.cpp"                , apath + "new_Test_.cpp"                ));
	EXPECT(files_equal(epath + "new_Test_.m"                  , apath + "new_Test_.m"                  ));

	EXPECT(files_equal(epath + "Point3_staticFunction.cpp"    , apath + "Point3_staticFunction.cpp"    ));
	EXPECT(files_equal(epath + "Point3_staticFunction.m"      , apath + "Point3_staticFunction.m"      ));
	EXPECT(files_equal(epath + "Point3_StaticFunctionRet.cpp" , apath + "Point3_StaticFunctionRet.cpp" ));
	EXPECT(files_equal(epath + "Point3_StaticFunctionRet.m"   , apath + "Point3_StaticFunctionRet.m"   ));

	EXPECT(files_equal(epath + "@Point2/argChar.cpp"          , apath + "@Point2/argChar.cpp"          ));
	EXPECT(files_equal(epath + "@Point2/argChar.m"            , apath + "@Point2/argChar.m"            ));
	EXPECT(files_equal(epath + "@Point2/argUChar.cpp"         , apath + "@Point2/argUChar.cpp"         ));
	EXPECT(files_equal(epath + "@Point2/argUChar.m"           , apath + "@Point2/argUChar.m"           ));
	EXPECT(files_equal(epath + "@Point2/dim.cpp"              , apath + "@Point2/dim.cpp"              ));
	EXPECT(files_equal(epath + "@Point2/dim.m"                , apath + "@Point2/dim.m"                ));
	EXPECT(files_equal(epath + "@Point2/Point2.m"             , apath + "@Point2/Point2.m"             ));
	EXPECT(files_equal(epath + "@Point2/returnChar.cpp"       , apath + "@Point2/returnChar.cpp"       ));
	EXPECT(files_equal(epath + "@Point2/returnChar.m"         , apath + "@Point2/returnChar.m"         ));
	EXPECT(files_equal(epath + "@Point2/vectorConfusion.cpp"  , apath + "@Point2/vectorConfusion.cpp"  ));
	EXPECT(files_equal(epath + "@Point2/vectorConfusion.m"    , apath + "@Point2/vectorConfusion.m"    ));
	EXPECT(files_equal(epath + "@Point2/x.cpp"                , apath + "@Point2/x.cpp"                ));
	EXPECT(files_equal(epath + "@Point2/x.m"                  , apath + "@Point2/x.m"                  ));
	EXPECT(files_equal(epath + "@Point2/y.cpp"                , apath + "@Point2/y.cpp"                ));
	EXPECT(files_equal(epath + "@Point2/y.m"                  , apath + "@Point2/y.m"                  ));
	EXPECT(files_equal(epath + "@Point3/norm.cpp"             , apath + "@Point3/norm.cpp"             ));
	EXPECT(files_equal(epath + "@Point3/norm.m"               , apath + "@Point3/norm.m"               ));
	EXPECT(files_equal(epath + "@Point3/Point3.m"             , apath + "@Point3/Point3.m"             ));

	EXPECT(files_equal(epath + "@Test/arg_EigenConstRef.cpp"  , apath + "@Test/arg_EigenConstRef.cpp"  ));
	EXPECT(files_equal(epath + "@Test/arg_EigenConstRef.m"    , apath + "@Test/arg_EigenConstRef.m"    ));
	EXPECT(files_equal(epath + "@Test/create_MixedPtrs.cpp"   , apath + "@Test/create_MixedPtrs.cpp"   ));
	EXPECT(files_equal(epath + "@Test/create_MixedPtrs.m"     , apath + "@Test/create_MixedPtrs.m"     ));
	EXPECT(files_equal(epath + "@Test/create_ptrs.cpp"        , apath + "@Test/create_ptrs.cpp"        ));
	EXPECT(files_equal(epath + "@Test/create_ptrs.m"          , apath + "@Test/create_ptrs.m"          ));
	EXPECT(files_equal(epath + "@Test/print.cpp"              , apath + "@Test/print.cpp"              ));
	EXPECT(files_equal(epath + "@Test/print.m"                , apath + "@Test/print.m"                ));
	EXPECT(files_equal(epath + "@Test/return_bool.cpp"        , apath + "@Test/return_bool.cpp"        ));
	EXPECT(files_equal(epath + "@Test/return_bool.m"          , apath + "@Test/return_bool.m"          ));
	EXPECT(files_equal(epath + "@Test/return_double.cpp"      , apath + "@Test/return_double.cpp"      ));
	EXPECT(files_equal(epath + "@Test/return_double.m"        , apath + "@Test/return_double.m"        ));
	EXPECT(files_equal(epath + "@Test/return_field.cpp"       , apath + "@Test/return_field.cpp"       ));
	EXPECT(files_equal(epath + "@Test/return_field.m"         , apath + "@Test/return_field.m"         ));
	EXPECT(files_equal(epath + "@Test/return_int.cpp"         , apath + "@Test/return_int.cpp"         ));
	EXPECT(files_equal(epath + "@Test/return_int.m"           , apath + "@Test/return_int.m"           ));
	EXPECT(files_equal(epath + "@Test/return_matrix1.cpp"     , apath + "@Test/return_matrix1.cpp"     ));
	EXPECT(files_equal(epath + "@Test/return_matrix1.m"       , apath + "@Test/return_matrix1.m"       ));
	EXPECT(files_equal(epath + "@Test/return_matrix2.cpp"     , apath + "@Test/return_matrix2.cpp"     ));
	EXPECT(files_equal(epath + "@Test/return_matrix2.m"       , apath + "@Test/return_matrix2.m"       ));
	EXPECT(files_equal(epath + "@Test/return_pair.cpp"        , apath + "@Test/return_pair.cpp"        ));
	EXPECT(files_equal(epath + "@Test/return_pair.m"          , apath + "@Test/return_pair.m"          ));
	EXPECT(files_equal(epath + "@Test/return_Point2Ptr.cpp"   , apath + "@Test/return_Point2Ptr.cpp"   ));
	EXPECT(files_equal(epath + "@Test/return_Point2Ptr.m"     , apath + "@Test/return_Point2Ptr.m"     ));
	EXPECT(files_equal(epath + "@Test/return_ptrs.cpp"        , apath + "@Test/return_ptrs.cpp"        ));
	EXPECT(files_equal(epath + "@Test/return_ptrs.m"          , apath + "@Test/return_ptrs.m"          ));
	EXPECT(files_equal(epath + "@Test/return_size_t.cpp"      , apath + "@Test/return_size_t.cpp"      ));
	EXPECT(files_equal(epath + "@Test/return_size_t.m"        , apath + "@Test/return_size_t.m"        ));
	EXPECT(files_equal(epath + "@Test/return_string.cpp"      , apath + "@Test/return_string.cpp"      ));
	EXPECT(files_equal(epath + "@Test/return_string.m"        , apath + "@Test/return_string.m"        ));
	EXPECT(files_equal(epath + "@Test/return_Test.cpp"        , apath + "@Test/return_Test.cpp"        ));
	EXPECT(files_equal(epath + "@Test/return_Test.m"          , apath + "@Test/return_Test.m"          ));
	EXPECT(files_equal(epath + "@Test/return_TestPtr.cpp"     , apath + "@Test/return_TestPtr.cpp"     ));
	EXPECT(files_equal(epath + "@Test/return_TestPtr.m"       , apath + "@Test/return_TestPtr.m"       ));
	EXPECT(files_equal(epath + "@Test/return_vector1.cpp"     , apath + "@Test/return_vector1.cpp"     ));
	EXPECT(files_equal(epath + "@Test/return_vector1.m"       , apath + "@Test/return_vector1.m"       ));
	EXPECT(files_equal(epath + "@Test/return_vector2.cpp"     , apath + "@Test/return_vector2.cpp"     ));
	EXPECT(files_equal(epath + "@Test/return_vector2.m"       , apath + "@Test/return_vector2.m"       ));
	EXPECT(files_equal(epath + "@Test/Test.m"                 , apath + "@Test/Test.m"                 ));

}

/* ************************************************************************* */
int main() { TestResult tr; return TestRegistry::runAllTests(tr); }
/* ************************************************************************* */
