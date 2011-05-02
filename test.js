//update these settings to match a test environment on your system
var path_to_dirfile = '/data/etc/defile.lnk'
var field_name = 'N18C04'
var extra_names = ['N19C05', 'N17C20']

//test the bindings
var bindings = require('./build/default/getdata_bindings')
var d = new bindings.Dirfile()
d.open('/data/etc/madeup', function (err) {
    console.log("opening /data/etc/madeup")
    if (err) {
      console.log(err) 
    } else { 
      console.log("success") 
    } })
d.open(path_to_dirfile, function (err) {
    console.log("opening " + path_to_dirfile)
    if (err) console.log(err) 
    else { 
      console.log("successfully opened") 
      console.log("trying to get nframes from " + path_to_dirfile)
      d.nframes(function (err, nf) {
	if (err) console.log(err)
	else {
	  console.log(nf)
	  console.log("trying to get some recent data: " + field_name)
	  d.getdata(field_name, nf-1, function(err, data) {
	    if (err) console.log(err)
	    else {
	      console.log(data);
	    } })
	} })
    } })

//test the listener
var listener = require('./dirfilewatcher')
var dfw = new listener.DirfileWatcher(path_to_dirfile)
dfw.addField(extra_names)
dfw.addField(field_name)
dfw.on('data', function(data) {
  console.log("Got new data")
  console.dir(data)
})
setTimeout(function() { 
    dfw.changePeriod(1000)
    dfw.test_str = "Changed string"
  }, 2500);
setTimeout(function() { 
    dfw.changePeriod(0)
  }, 5000)
