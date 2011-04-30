//update these settings to match a test environment on your system
var path_to_dirfile = '/data/etc/defile.lnk'
var field_name = 'N18C04'

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

