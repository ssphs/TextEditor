#include "graphics/TextEditorWindow.hpp"

#include <iomanip>

TextEditorWindow::~TextEditorWindow()
{
    // Free the target
    if(_target != 0) SDL_FreeSurface(_target);

    // Delete all EditorChar pointers
    for(unsigned int y = 0; y < _lines.size(); ++y)
        delete _lines[y];
}

bool TextEditorWindow::init(int w, int h)
{
    bool r = this->resize(w, h);
    std::cout << "Initialised TextEditorWindow with size " << w << "*" << h << std::endl;
    _lines.push_back(new CharList); // Make sure we have one line to start with
    _colors.load("colorschemes/default.csch");
    _tabLen = 4;

    return r;
}

bool TextEditorWindow::resize(int w, int h)
{
    // Free the target to avoid memory leaks
    if(_target != 0) SDL_FreeSurface(_target);

    // Create new target, making sure to use the right architecture
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        _target = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
    #else
        _target = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    #endif

    // Something went wrong
    if(_target == 0)
    {
        std::cerr << "CreateRGBSurface failed: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

void TextEditorWindow::moveCursorDown()
{
    // We can't really move down if we're at the bottom, eh?
    if(_cursorY < _lines.size() - 1)
    {
        ++_cursorY; // Move down

        // Make sure that we're not trying to access non-existing chars
        if(_cursorX >= _lines[_cursorY]->size())
            _cursorX = _lines[_cursorY]->size();
    }
}

void TextEditorWindow::moveCursorUp()
{
    if(_cursorY > 0)
    {
        --_cursorY;

        // Make sure that we're not trying to access non-existing chars
        if(_cursorX >= _lines[_cursorY]->size())
            _cursorX = _lines[_cursorY]->size();
    }
}

void TextEditorWindow::moveCursorRight()
{
    // Check that we're not moving further than the line is long... what?
    if(_cursorX < _lines[_cursorY]->size())
        ++_cursorX;
    else
        // Attempt to move down if the end of the line is reached
        this->moveCursorDown();
}

void TextEditorWindow::moveCursorLeft()
{
    // Check if we're trying to move into a wall
    if(_cursorX > 0)
        --_cursorX;
    else
    {   // If we are, attempt to move up
        unsigned int oldCY = _cursorY;
        this->moveCursorUp();

        // Move to the end of the line if the up-move was successful
        if(_cursorY != oldCY)
            _cursorX = _lines[_cursorY]->size();
    }
}

void TextEditorWindow::addChar(char c)
{
    // std::cout << "Adding char '" << c << "' at (" << _cursorX << ", " << _cursorY << ")" << std::endl;
    _lines[_cursorY]->insert(_lines[_cursorY]->begin() + _cursorX, c); // Insert the char

    // std::cout << "Char at (" << _cursorX << ", " << _cursorY << "): '" << (*(_lines[_cursorY]))[_cursorX] << "'" << std::endl;
    this->moveCursorRight(); // Go to the right
}

void TextEditorWindow::removeChar()
{
    if(_cursorX > 0)
    {
        // std::cout << "Removing char at (" << (_cursorX - 1) << ", " << _cursorY << ")" << std::endl;
        _lines[_cursorY]->erase(_lines[_cursorY]->begin() + (_cursorX - 1));
        this->moveCursorLeft();
    }
    else
    {
        // Make sure we're not moving stuff to line -1
        if(_cursorY > 0)
        {
            unsigned int oldSize = _lines[_cursorY - 1]->size();

            // Move current line above
            for(unsigned int x = 0; x < _lines[_cursorY]->size(); ++x)
            {
                // We can push since we're appending to the end of the line
                _lines[_cursorY - 1]->push_back((*(_lines[_cursorY]))[x]);
            }

            // Remove old line
            _lines.erase(_lines.begin() + _cursorY);

            this->moveCursorUp();
            _cursorX = oldSize;
        }
    }
}

void TextEditorWindow::addLine()
{
    _lines.insert(_lines.begin() + (_cursorY + 1), new CharList());

    // Move the rest of the current line to the one below
    ++_cursorX;
    for(unsigned int x = _cursorX; x < _lines[_cursorY]->size(); ++x)
        _lines[_cursorY + 1]->push_back((*(_lines[_cursorY]))[x]);

    for(unsigned int x = _cursorX; x < _lines[_cursorY]->size(); ++x)
        _lines[_cursorY]->erase(_lines[_cursorY]->begin() + x);

    this->moveCursorDown();
    // Make sure that we're not trying to access non-existing chars
    if(_cursorX >= _lines[_cursorY]->size())
        _cursorX = _lines[_cursorY]->size();
}

void TextEditorWindow::onKeyEvent(SDL_KeyboardEvent &key, bool dir)
{
    // Toggle capslock
    if(key.keysym.sym == SDLK_CAPSLOCK)
    {
        _capslock = !_capslock;
        return;
    }

    // Get the name of the key
    char kName = (char) key.keysym.unicode;
    /*
    if(_capslock)
        text::TextUtils::toUppercase(kName);
    //*/

    if(dir) // DOWN
    {
        // std::cout << "Key " << (int) key.keysym.sym << " AKA '" << kName.c_str() << "' was pressed." << std::endl;

        bool shallPrint = true;

        if(key.keysym.sym == SDLK_DOWN)
        {
            this->moveCursorDown();
        }
        else if(key.keysym.sym == SDLK_LEFT)
        {
            this->moveCursorLeft();
        }
        else if(key.keysym.sym == SDLK_RIGHT)
        {
            this->moveCursorRight();
        }
        else if(key.keysym.sym == SDLK_UP)
        {
            this->moveCursorUp();
        }
        else if(key.keysym.sym == SDLK_BACKSPACE)
        {
            this->removeChar();
        }
        else if(key.keysym.sym == SDLK_RETURN)
        {
            this->addLine();
        }
        else if(key.keysym.sym == SDLK_TAB)
        {
            for(unsigned int i = 0; i < _tabLen; ++i)
                addChar(' ');
        }
        else
        {
            // Get the unicode char
            if(kName < 0x80 && kName > 0)
            {
                this->addChar(kName);
            }
            else
            {
                // Unable to find character name
                /* ========================== */
                /*            TODO            */
                /* ========================== */

                shallPrint = false;
            }
        }

        // Print the entire line
        if(shallPrint)
        {
            char line[_lines[_cursorY]->size() + 1];
            for(unsigned int x = 0; x < _lines[_cursorY]->size(); ++x)
            {
                line[x] = (*(_lines[_cursorY]))[x];
            }
            line[_lines[_cursorY]->size()] = '\0';

            std::vector<const char*> keywords;
            keywords.push_back("for");
            keywords.push_back("unsigned");
            keywords.push_back("int");
            keywords.push_back("char");

            std::string sLine(line);
            applySyntaxHighlighting(sLine, keywords);
            // std::cout << sLine.c_str() << std::endl;
            std::vector<text::EditorChar> chars = getEditorCharVector(sLine, _colors);
            printEditorChars(chars);
        }
    }
    else // UP
    {
        // std::cout << "Key " << (int) key.keysym.sym << " AKA '" << kName.c_str() << "' was released." << std::endl;
    }
}
