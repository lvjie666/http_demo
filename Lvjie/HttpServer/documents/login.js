// var xmlHttp;

// function createXmlHttpRequest(){
//     if(window.XMLHttpRequest){
//         xmlHttp = new XMLHttpRequest();
//     }else if(window.ActiveXObject){
//         xmlHttp = new ActiveXObject("Microsoft.XMLHTTP");
//     }
// }

// function login(){
//     createXmlHttpRequest();
//     var name = document.getElementById("username").value;
//     var password = document.getElementById("password").value;
//     if(name == null || name == ""){
//         innerHtml("please input your account");
//         return;
//     }
//     if(password == null || password == ""){
//         innerHtml("please input your password");
//         return;
//     }
//     var url = "user.php";
//     //xmlHttp.open("POST",url,true);
//     xmlHttp.open("GET",url,true);
//     xmlHttp.onreadystatechange = handleResult;
//     xmlHttp.setRequestHeader('Content-Type','application/x-www-form-urlencoded');
//     //xmlHttp.send("&name=" + name + "&psd=" + password);
//     xmlHttp.send(null);
// }

// function handleResult(){
//     if(xmlHttp.readyState == 4 && xmlHttp.status == 200){
//         var response = xmlHttp.responseText;
//         var json = eval('(' + response + ')');
//         if(json['login_result']){
//             alert("login success!");
//         }else{
//             innerHtml("account/password is error");
//         }
//     }
// }

// function innerHtml(message){
//     document.getElementById("tip").innerHTML = "<span style='font-size:12px;color:red;'>" + message + "</span>";
// }
function success(text) {
    let reqBtn=document.querySelector("#introduction");
    reqBtn.innerText = text;
}

function login(){
var request = new XMLHttpRequest(); // 新建XMLHttpRequest对象

request.onreadystatechange = function () { // 状态发生变化时，函数被回调
    if (request.readyState === 4) { // 成功完成
        // 判断响应结果:
        if (request.status === 200) {
            // 成功，通过responseText拿到响应的文本:
            return success(request.responseText);
        } else {
            // 失败，根据响应码判断失败原因:
            return success(request.status);
        }
    } else {
        // HTTP请求还在继续...
    }
}

// 发送请求:
request.open('GET', 'test.txt',true);
request.send();
}
