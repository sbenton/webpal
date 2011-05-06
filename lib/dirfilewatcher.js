// Module to peirodically query a dirfile with GetData, and report changes

var bindings = require('./getdata_bindings')
  , events = require('events')
  , util = require ('util')

var DirfileWatcher = function(path) {
  this.d = new bindings.Dirfile()
  this.period = 500	  //refresh period in ms
  this.fields = []	  //fields to get the value of
  this.last_nframes = 0	  //last value of nframes
  this.count = 0	  //field count. Used for tracking parallel calls

  //open file and start timeout to check data
  var self=this
  this.d.open(path, function(err) {
    //TODO check error state (elsewhere too)
    self.intervalid = setInterval(self.check, self.period, self)
  })
}

util.inherits(DirfileWatcher, events.EventEmitter)

//add a field name or array of field names to those watched for data
DirfileWatcher.prototype.addField = function(f) {
  if (f instanceof Array) {
    this.fields = this.fields.concat(f)
  } else {
    this.fields.push(f)
  }
}

//change dirfile poll period in ms (0 stops loop)
DirfileWatcher.prototype.changePeriod = function(period) {
  this.period = period
  clearInterval(this.intervalid);
  if (period > 0) {
    this.intervalid = setInterval(this.check, this.period, this)
  }
}

//callback to close field name for getdata callbacks
function makeGDCallback(self, field, retval) {
  return function(err, data) {
    /*console.log("getdata(%s,%d)=%d %d/%d", field, self.last_nframes-1,
	data, self.count, self.fields.length)*/
    //TODO report only data that changes to save data?
    if (!err) retval[field] = data
    if (--self.count == 0) self.emit('data', retval)
  }
}

//timer callback, needs a reference to self
DirfileWatcher.prototype.check = function(self) {
  //console.log("Checking data, last_nframes %d", self.last_nframes)
  if (!self.fields.length) return   //don't do anything if not watching fields
  self.d.nframes(function(err, n) {
    if (n > self.last_nframes) {
      var retval = {}
      self.count = self.fields.length
      self.last_nframes = n;
      for (var i_field in self.fields) {
	var field = self.fields[i_field]
	self.d.getdata(field, self.last_nframes-1, 
	    makeGDCallback(self, field, retval))
      }
    }
  })
}

exports.DirfileWatcher = DirfileWatcher
