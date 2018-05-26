string read_file(const string& file_name)
{
    ifstream is(file_name);
    if (is.fail()) {
        cout << "Error reading file: " << file_name << endl;
        exit(1);
    }

    is.seekg(0, ios::end);
    int file_size = is.tellg();
    is.seekg(0, ios::beg);
    file_size -= is.tellg();

    string data;
    data.resize(file_size);

    is.read(const_cast<char*>(data.data()), file_size);
    is.close();

    return data;
}
