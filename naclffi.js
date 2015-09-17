var NaclFFI = (function() {
    function resolve(cachemap, x) {
        if (typeof x == "object") {
            return x[0];
        } else {
            return cachemap[x];
        }
    }

    function handleMessage(msg) {
        // console.log(msg.data);
        var cachemap = this.cachemap;
        var obj = typeof resolve(cachemap, msg.data.obj) != "undefined" ? resolve(cachemap, msg.data.obj) : window;
        var ret = typeof msg.data.args == "object" ? obj[msg.data.method].apply(obj, msg.data.args.map(function (x) { return resolve(cachemap, x); })) : obj[msg.data.method];
        if (msg.data.ret) {
            if (typeof ret != "object") {
                this.postMessage({"type": "raw", "data": ret});
            } else {
                cachemap.push(ret);
                this.postMessage({"type": "jsref", "data": cachemap.length - 1});
            }
        }
    }

    return {
        apply: function(embed) {
            embed.cachemap = [embed];
            embed.addEventListener('message', function (msg) { handleMessage.call(embed, msg); }, true);
            embed.destroyJsref = function(id) { delete embed.cachemap[id]; };
            embed.registerCallback = function() {
            var id = embed.cachemap.length;
            embed.cachemap.push(function() { 
                embed.postMessage({"type": "call", "id": id, "data": arguments});
            });
            return id;
            }
        }
    };
})();
