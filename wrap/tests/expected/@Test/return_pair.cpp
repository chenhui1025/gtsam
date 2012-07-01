// automatically generated by wrap
#include <wrap/matlab.h>
#include <set>
#include <folder/path/to/Test.h>
using namespace geometry;
typedef boost::shared_ptr<Test> Shared;
void mexFunction(int nargout, mxArray *out[], int nargin, const mxArray *in[])
{
  checkArguments("return_pair",nargout,nargin-1,2);
  mxArray* mxh = mxGetProperty(in[0],0,"self");
  Shared* self = *reinterpret_cast<Shared**> (mxGetPr(mxh));
  Shared obj = *self;
      Vector v = unwrap< Vector >(in[1]);
      Matrix A = unwrap< Matrix >(in[2]);
  pair< Vector, Matrix > result = obj->return_pair(v,A);
  out[0] = wrap< Vector >(result.first);
  out[1] = wrap< Matrix >(result.second);
}
