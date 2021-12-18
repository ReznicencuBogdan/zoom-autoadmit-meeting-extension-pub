# zoom-autoadmit-meeting-extension-pub
A tool for Zoom that helps manage meetings.


## What is provided and what to use:
* generate_jwt.py `generate a jwt token using zoom dev credential` - edit this script with keys provided by zoom
* paste the jwt token in the `database.json` file
* modify the `SDKInterfacePlugin.cpp` and edit the following defines:
-	ZOOM_MAIL
-	ZOOM_PWS

## About the `database.json` file:
* Structure
```json
"users": [
    {
        "cvrtto": "Michael Dev",
        "fautoadmit": true,
        "fban": false,
        "fcvrt": true,
        "name": "Chair. Conto - Speaker"
    }
```
* `name` : `string` - the name one person ussualy enters the meeting
* `fautoadmit` : `true|false` - this person will skip the waiting room
* `fcvrt` : `true|false` - this person has another name - consider changing the name using `cvrtto`
* `cvrtto` : `string` - the name this person will have from now on in this meeting
* `fban` : `true|false` - this person cannot enter the meeting

## This is just an example - testing the zoom development kit
- Follow the indications provided by the Zoom development team. I provided only the source code but there are some steps that you must follow in order to setup a working solution.
- Note: this is just a test project. As such, not much effort was put in making this 'piece of art'. There are some issues regarding the encoding of the name. The json parser doesn't specifficaly work with wchar_t* (PWCHAR) or std::wstring. Therefore I resorted to truncating the the string's chars and disregarding the string encoding.