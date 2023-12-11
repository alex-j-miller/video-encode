/*
* main.cpp
* @author: Alex Miller
* @version: 1.0
* @date: Dec 11 2023
* @description: Main file for video steganography project to store files in videos
* @usage: ./main
*/

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <bitset>

#include <opencv2/opencv.hpp>
#include "lodepng.h"

using namespace std;
using namespace cv;

const unsigned int WIDTH = 1920;
const unsigned int HEIGHT = 1080;
const unsigned int PIXEL_SIZE = 4;
const string IMAGE_DIRECTORY = "images/";
const string OUTPUT_DIRECTORY = "output_images/";
const unsigned int NUM_BYTES = ((WIDTH * PIXEL_SIZE) * (HEIGHT * PIXEL_SIZE)) / 4;
const unsigned int FRAMES_PER_IMAGE = 1;

unsigned int numPNG = 0;
bool endfile = false;
string fileToEncode = "test.mp4";

/*
* Main function of execution, calls encode() or decode() depending on user input
*/
void main () {
    char input = '';
	cout << "Do you want to encode (e) or decode(d) or both (b) : ";
	cin >> input;

	if (input == 'e') {
		encode();
	}
	else if (input == 'd') {
		decode();
	}
	else if (input == 'b') {
		encode();
		decode();
	}
	else {
		cout << "Invalid input" << endl;
	}

	return 0;
}

/*
* Function to call for encoding a file into a video
*/
void encode () {
    vector<unsigned char> image;
    vector<unsigned char> bytes;

    string filename = "files/" + fileToEncode;

    filesystem::remove_all(IMAGE_DIRECTORY);
    filesystem::create_directory(IMAGE_DIRECTORY);

    while (!endfile) {
        string outputName = to_string(numPNG) + ".png";
        bytes = getNthSetOfBytes(PIXEL_SIZE * WIDTH * HEIGHT, filename);
        image = generateImageVector(bytes);
        generatePNG(image, outputName);
        numPNG++;
    }

    cout << numPNG << " images saved." << endl;
    generateVideo();
    return;
}

/*
* Function to call for decoding a video into a file
*/
void decode () {
    vector<unsigned char> bytes;
    filesystem::remove_all(OUTPUT_DIRECTORY);
    generatePNGSequence("videos/output.mp4");
    filesystem::remove("files/outfile" + outfileExtension);

    string pictureName;
    for (unsigned int i = 0; i < numPNG; i++) {
        pictureName = OUTPUT_DIRECTORY + to_string(i) + ".png";
        bytes = PNGToData(pictureName);
        appendBytesToFile(bytes, "files/outfile" + outfileExtension);
    }
}

/*
* Appends a vector of bytes to a file
*
* @param bytes - vector of bytes to be appended to the file
* @param fileName - the name of the file to be appended to
*/
void appendBytesToFile (const vector<unsigned char>& bytes, const string filename) {
    ofstream file(filename, ios::binary | ios::app);
    if(!file) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    if(!file) {
        cerr << "Error writing bytes to file." << endl;
        return;
    }

    file.close();
}

/*
* Converts a PNG file into a vector of bytes
*
* @param bytes - vector of bytes to be appended to the file
* @param fileName - the name of the file to be appended to
* @return a vector of bytes representing the nth set of bytes from the file
*/
vector<unsigned char> PNGToData(const string& filename) {
    vector<unsigned char> bytes;
    unsigned char bytes = 0;
    Mat image = imread(filename);
    if (image.empty()) {
        cerr << "Error reading image." << endl;
        return bytes;
    }

    Vec3b color;
    int byteCounter = 0;
    for (unsigned int y = PIXEL_SIZE / 2; y < image.rows; y += PIXEL_SIZE) {
        for (unsigned int x = PIXEL_SIZE / 2; x < image.cols; x += PIXEL_SIZE) {
            color = image.at<Vec3b>(x, y);
            bytes = byte << 2;

            // White marks the end of the file
            if (static_cast<int>(color[0]) > tolerance && static_cast<int>(color[1]) > tolerance && static_cast<int>(color[2]) > tolerance) {
                return bytes;
            } else if (static_cast<int>(color[2]) > tolerance) { // Red 01
                bytes |= 1;
            } else if (static_cast<int>(color[0]) > tolerance) { // Green 11
                bytes |= 3;
            } else if (static_cast<int>(color[1]) > tolerance) { // Blue 10
                bytes |= 2;
            } else { // Black 00
                bytes |= 0;
            }

            if (byteCounter == 3) {
                bytes.push_back(byte);
                byteCounter = 0;
            } else {
                byteCounter++;
            }
        }
    }
    return bytes;
}

/*
* Generates a PNG sequence from a video
*
* @param filename - the name of the video file to be read from
*/
void generatePNGSequence (const string& filename) {
    VideoCaputure video(videoPath);
    if(!video.isOpened()) {
        cerr << "Error opening video file." << endl;
        return;
    }

    int frameCount = static_cast<int>(video.get(CAP_PROP_FRAME_COUNT));
    int frameNumber = 0;

    filesystem::create_directory(OUTPUT_DIRECTORY);

    while (frameNumber < frameCount) {
        Mat frame;
        if (!video.read(frame)) {
            cerr << "Error reading frame." << endl;
            break;
        }

        string outputName = OUTPUT_DIRECTORY + to_string(frameNumber) + ".png";
        if (!imwrite(outputName, frame)) {
            cerr << "Error writing frame." << endl;
            break;
        }

        frameNumber++;
    }

    video.release();

    cout << "PNG sequence saved. Total frames: " << frameNumber << endl;
    numPNG = frameNumber;
}

/*
* Reads from the directory of generated images and generates a video from them
*/
void generateVideo () {
    VideoWriter video("video.avi", VideoWriter::fourcc('m', 'p', '4', 'v'), 10, Size(WIDTH, HEIGHT));

    if (!video.isOpened()) {
        cerr << "Error opening video file." << endl;
        return;
    }

    Mat frame;
    for (unsigned int i = 0; i < numPNG; i++) {
        string imagePath = IMAGE_DIRECTORY + to_string(i) + ".png";
        frame = imread(imagePath);

        if (frame.empty()) break;

        for (unsigned int j = 0; j < FRAMES_PER_IMAGE; j++) {
            video.write(frame);
        }
    }

    video.release();
    cout << "Video saved." << endl;
}

/*
* Generates a PNG file freom an image vector
* 
* @param image - vector of unsigned chars representing an image in the form of {R, G, B, A, R, G, B, A, ...}
* @param fileName - the name of the file to be generated
* @return 0 if successful, 1 if unsuccessful
*/
int generatePNG (const vector<unsigned char>& image, const string& fileName) {
    const string OUTPUT_PATH = IMAGE_DIRECTORY + fileName;
    if (lodepng::encode(OUTPUT_PATH, image, WIDTH, HEIGHT) != 0) {
        cerr << "Error generating PNG." << endl;
        return 1;
    }

    return 0;
}

/*
* Returns the nth set of bytes of size NUM_BYTES (number of bytes that will fit within the image) from a file
* 
* @param n - the nth set of bytes to be returned
* @param fileName - the name of the file to be read from
* @return result - vector of bytes representing the nth set of bytes from the file
*/
vector<unsigned char> getNthSetOfBytes (const unsigned int& n, const string& fileName) {
    vector<unsigned char> result;
    unsigned int bytesToUse = NUM_BYTES;

    ifstream file(fileName, ios::binary);
    if (!file) {
        cerr << "Error opening file: " << fileName << endl;
        return result;
    }

    // Get the size of the file
    file.seekg(0, ios::end);
    streampos fileSize = file.tellg();
    file.seekg(0, ios::beg);

    // Calculate the start position of the nth set of bytes
    streampos startPosition = n * bytesToUse;

    // Read the bytes into the result vector
    vector<unsigned char> buffer(bytesToUse);
    file.read(reinterpret_cast<char*>(buffer.data()), bytesToUse);
    if (!file) {
        cerr << "Failed to read bytes from file." << endl;
        return result;
    }

    // Copy the read bytes into the result vector
    result.assign(buffer.begin(), buffer.end());

    return result;
}

/*
* Generates an image vector from a vector of bytes
*
* @param bytes - vector of bytes to be converted into an image
* @return image - vector of unsigned chars representing an image in the form of {R, G, B, A, R, G, B, A, ...}
*/
vector<unsigned char> generateImageVector (const vector<unsigned char>& bytes) {
    vector<unsigned char> image(WIDTH * HEIGHT * 4);
    int gridPosition = 0;
    for (unsigned int y = 0; y < HEIGHT; y++) {
        for (unsigned int x = 0; x < WIDTH; x++) {
            unsigned int pixelIndex = (y * WIDTH + x) * 4;

            // Pixels are PIXEL_SIZE x PIXEL_SIZE 
            gridPosition = (x / PIXEL_SIZE) + (y / PIXEL_SIZE) * (WIDTH / PIXEL_SIZE);

            int bits = 0;
            if ((gridPosition / 4) < bytes.size()) {
                // Takes rightmost 2 bits after bit shifting accounting for grid position
                bits = bytes[gridPosition / 4] >> (6 - ((gridPosition % 4) * 2)) & 3;
                switch (bits)
                {
                case 0:
                    image[pixelIndex] = 0;
                    image[pixelIndex + 1] = 0;
                    image[pixelIndex + 2] = 0;
                    image[pixelIndex + 3] = 255;
                    break;
                case 1:
                    image[pixelIndex] = 255;
                    image[pixelIndex + 1] = 0;
                    image[pixelIndex + 2] = 0;
                    image[pixelIndex + 3] = 255;
                    break;
                case 2:
                    image[pixelIndex] = 0;
                    image[pixelIndex + 1] = 255;
                    image[pixelIndex + 2] = 0;
                    image[pixelIndex + 3] = 255;
                    break;
                case 3:
                    image[pixelIndex] = 0;
                    image[pixelIndex + 1] = 0;
                    image[pixelIndex + 2] = 255;
                    image[pixelIndex + 3] = 255;
                    break;
                }
            } else {
                image[pixelIndex] = 255;
                image[pixelIndex + 1] = 255;
                image[pixelIndex + 2] = 255;
                image[pixelIndex + 3] = 255;
            }
        }
    }
    return image;
}