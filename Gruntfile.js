module.exports = function(grunt) {

    grunt.loadTasks('tasks');

    grunt.initConfig({
        compile: {
            ws: {
                options: {
                    compileOnly: true
                },
                main: ['src/WebSocket.cpp'],
                dest: 'compiled/WebSocket.o',
                links: [
                    'lib/libwebsockets/lib'
                ]
            },
            run: {
                options: {
                    verbose: false
                },
                main: ['test/main.cpp', 'tmp/WebSocketClient.cpp'],
                dest: 'socket.io',
                links: [
                    'lib/libwebsockets/lib',
                    'tmp'
                ]
            }
        }
    });

    grunt.registerTask('default', ['compile:ws']);
    grunt.registerTask('run', ['compile:run']);

};
