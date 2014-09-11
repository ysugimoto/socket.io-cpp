var child = require('child_process');
var path  = require('path');

module.exports = function(grunt) {

    grunt.registerMultiTask('compile', 'C++ compile', function() {
        var main = this.data.main;
        var libs = this.data.links;
        var dest = this.data.dest;
        var cmd  = 'g++ -c -std=c++11 -stdlib=libc++';
        var opts = [];
        var done = this.async();
        var options = this.options({
            flags: []
        });

        options.flags.forEach(function(flag) {
            opts.push(flag);
        });

        libs.forEach(function(lib) {
            opts.push('-I ' + path.resolve(process.cwd(), lib));
        });

        cmd = cmd + ' ' + opts.join(' ') + ' ' + main + ' -o ' + dest;

        grunt.log.writeln('Compile command: ' + cmd);

        var proc = child.exec(cmd);
        proc.stdout.on('data', function(chunk) {
            grunt.log.writeln(chunk);
        });
        proc.stderr.on('data', function(chunk) {
            grunt.log.errorlns(chunk);
        });

        proc.on('end', function() {
            done();
        });
    });
};
