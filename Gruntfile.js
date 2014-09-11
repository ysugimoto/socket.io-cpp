module.exports = function(grunt) {

    grunt.loadTasks('tasks');

    grunt.initConfig({
        compile: {
            ws: {
                main: 'src/WebSocket.cpp',
                dest: 'socket.io.o',
                links: [
                    'lib/libwebsockets/lib'
                    //'lib/websocketpp/*.hpp',
                    //'lib/websocketpp/*.cpp',
                    //'lib/websocketpp/**/*.h',
                    //'lib/websocketpp/**/*.hpp',
                    //'lib/websocketpp/**/*.cpp',
                    //'lib/rapidjson/include/rapidjson/**/*'
                    //'lib/rapidjson/include/rapidjson/*.h'
                    //'lib/rapidjson/include/rapidjson/internal/*.h'
                ]
            }
        }
    });

    grunt.registerTask('default', ['compile']);

};
