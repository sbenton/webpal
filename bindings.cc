/* This code is PUBLIC DOMAIN, and is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND. See the accompanying 
 * LICENSE file.
 */

#include <v8.h>
#include <node.h>

#include <stdlib.h>
#include <string>
#include <getdata.h>

/* Argument checking macros
 *  Check index I of Argument array for correct type, assigns to VAR
 *  NB: dangerous macros. Not wrapped in do..while block because of declarations
 */
#define REQ_FUN_ARG(I, VAR)			                            \
  if (args.Length() <= (I) || !args[I]->IsFunction())	                    \
    return v8::ThrowException(v8::Exception::TypeError(                     \
                  v8::String::New("Argument " #I " must be a function")));  \
  v8::Local<v8::Function> VAR = v8::Local<v8::Function>::Cast(args[I]);

#define REQ_STR_ARG(I, VAR)						    \
  if (args.Length() <= (I) || !args[I]->IsString())			    \
    return v8::ThrowException(v8::Exception::TypeError(                     \
                  v8::String::New("Argument " #I " must be a string")));    \
  v8::String::Utf8Value VAR(args[I]->ToString());

#define REQ_INT_ARG(I, VAR)						    \
  if (args.Length() <= (I) || !args[I]->IsNumber())			    \
    return v8::ThrowException(v8::Exception::TypeError(                     \
                  v8::String::New("Argument " #I " must be a number")));    \
  int64_t VAR = args[I]->IntegerValue();

class Dirfile: node::ObjectWrap
{
private:
  DIRFILE* d_;
public:

  static void Init(v8::Handle<v8::Object> target)
  {
    v8::HandleScope scope;

    v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(New);

    //NB: hello world example had a static persistent template used here
    t->InstanceTemplate()->SetInternalFieldCount(1);  //field used by (un)wrap
    t->SetClassName(v8::String::NewSymbol("Dirfile"));

    //setup Dirfile object methods
    NODE_SET_PROTOTYPE_METHOD(t, "open", Open);
    NODE_SET_PROTOTYPE_METHOD(t, "getdata", GetData);
    NODE_SET_PROTOTYPE_METHOD(t, "nframes", Nframes);

    target->Set(v8::String::NewSymbol("Dirfile"), t->GetFunction());
  }

  Dirfile() : d_(NULL)
  {
  }

  ~Dirfile()
  {
  }

  static v8::Handle<v8::Value> New(const v8::Arguments& args)
  {
    v8::HandleScope scope;
    Dirfile* df = new Dirfile();
    df->Wrap(args.This());
    return args.This();
  }

  enum res_t {res_none, res_int, res_double};
  struct dirfile_request_t {
    Dirfile* d;
    v8::Persistent<v8::Function> cb;
    std::string name;
    int64_t num;
    enum res_t rt;
    union { int64_t ival; double dval; } result;
  };

  // Generic dirfile request handler, calls handler fn
  // i_name, i_num, i_fun are indexes of params in args (-1 if not used)
  static v8::Handle<v8::Value> DirfileRequest(int (*fn)(eio_req*),
      const v8::Arguments& args, int i_name, int i_num, int i_fun)
  {
    v8::HandleScope scope;
    Dirfile* d = ObjectWrap::Unwrap<Dirfile>(args.This());
    dirfile_request_t* dreq = new dirfile_request_t();

    dreq->d = d;
    REQ_FUN_ARG(i_fun, cb); //must always have callback
    dreq->cb = v8::Persistent<v8::Function>::New(cb);
    if (i_name >= 0) {
      REQ_STR_ARG(i_name, name);
      dreq->name.assign(*name);
    }
    if (i_num >= 0) {
      REQ_INT_ARG(i_num, num);
      dreq->num = num;
    }
    dreq->rt = res_none;    //initialize in case handler doesn't set

    dreq->d->Ref();
    eio_custom(fn, EIO_PRI_DEFAULT, EIO_After, dreq);
    ev_ref(EV_DEFAULT_UC);
    return v8::Undefined();
  }

  // Generic post-processing function
  // Checks error state and return value, then starts callback
  static int EIO_After(eio_req *req)
  {
    v8::HandleScope scope;
    dirfile_request_t *dreq = static_cast<dirfile_request_t *>(req->data);
    ev_unref(EV_DEFAULT_UC);
    dreq->d->Unref();

    v8::Local<v8::Value> argv[2];
    int argc;

    if (gd_error(dreq->d->d_) == GD_E_OK) { //success, pass null error to cb
      argv[0] = v8::Local<v8::Value>::New(v8::Null());
    } else {				    //failure, pass gd_error_string
      char *errstr = gd_error_string(dreq->d->d_, NULL, 0);
      argv[0] = v8::Exception::Error(v8::String::New(errstr));
      free(errstr);
    }
    
    if (dreq->rt == res_none) {
      argc = 1;
    } else if (dreq->rt == res_int) {
      argc = 2;
      argv[1] = v8::Integer::New(dreq->result.ival);
    } else if (dreq->rt == res_double) {
      argc = 2;
      argv[1] = v8::Number::New(dreq->result.dval);
    }

    v8::TryCatch try_catch;
    dreq->cb->Call(v8::Context::GetCurrent()->Global(), argc, argv);
    if (try_catch.HasCaught()) {
      node::FatalException(try_catch);
    }

    dreq->cb.Dispose();
    delete dreq;
    return 0;
  }

  static v8::Handle<v8::Value> Open(const v8::Arguments& args)
  {
    return DirfileRequest(EIO_Open, args, 0, -1, 1);
  }
  //TODO can this return void?
  static int EIO_Open(eio_req *req)
  {
    dirfile_request_t* dreq = static_cast<dirfile_request_t*>(req->data);
    dreq->d->d_ = gd_open(dreq->name.c_str(), GD_RDONLY);
    return 0;
  }


  static v8::Handle<v8::Value> Nframes(const v8::Arguments& args)
  {
    return DirfileRequest(EIO_Nframes, args, -1, -1, 0);
  }
  static int EIO_Nframes(eio_req *req)
  {
    dirfile_request_t* dreq = static_cast<dirfile_request_t*>(req->data);
    dreq->rt =res_int;
    dreq->result.ival = gd_nframes(dreq->d->d_);
    return 0;
  }

  //TODO if  binary Buffers added, make GetData fetch more than single sample
  static v8::Handle<v8::Value> GetData(const v8::Arguments& args)
  {
    return DirfileRequest(EIO_GetData, args, 0, 1, 2);
  }
  static int EIO_GetData(eio_req *req)
  {
    dirfile_request_t* dreq = static_cast<dirfile_request_t*>(req->data);
    dreq->rt =res_double;
    gd_getdata(dreq->d->d_, dreq->name.c_str(), dreq->num,
	0, 0, 1, GD_FLOAT64, &(dreq->result.dval));
    return 0;
  }


};

extern "C" {
  static void init (v8::Handle<v8::Object> target)
  {
    Dirfile::Init(target);
  }
  NODE_MODULE(getdata_bindings, init);
}
