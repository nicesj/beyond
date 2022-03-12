package com.samsung.android.beyond;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.ImageDecoder;
import android.util.Log;

import androidx.annotation.ColorInt;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.FloatBuffer;
import java.util.HashMap;
import java.util.Random;

public final class TestUtils {

    public static final String TAG = "BEYOND_TEST";

    public static final String TEST_SERVER_IP = "127.0.0.1";

    public static final int TEST_SERVER_PORT = 3000;

    public static final String TEST_UINT8_MODEL_NAME = "mobilenet_v1_1.0_224_quant.tflite";

    public static final String TEST_FLOAT32_MODEL_NAME = "posenet-model-mobilenet_v1_100.tflite";

    public static final int MOBILENET_INPUT_TENSOR_NUMBER = 1;

    public static final int MOBILENET_OUTPUT_TENSOR_NUMBER = 1;

    public static final int[] MOBILENET_INPUT_TENSOR_DIMENSIONS = {1, 224, 224, 3};

    public static final int[] MOBILENET_OUTPUT_TENSOR_DIMENSIONS = {1, 1001};

    public static final int POSENET_INPUT_TENSOR_NUMBER = 1;

    public static final int[] POSENET_INPUT_TENSOR_DIMENSIONS = {1, 257, 257, 3};

    public static final int[] TEST_DIMENSIONS = {1, 2, 3, 4};

    public static ByteBuffer TEST_DATA_UINT8;

    public static FloatBuffer TEST_DATA_FLOAT32;

    public static ByteBuffer TEST_ORANGE_IMAGE;

    public static final String ORANGE_IMAGE_FILE_NAME = "orange.png";

    public static final String LABEL_FILE_NAME = "labels_mobilenet_quant_v1_224.txt";

    public static HashMap<Integer, String> labelMap = new HashMap<>();

    public static String getModelAbsolutePath(Context context, String modelName) {
        File modelFile = getFileFromAsset(context, modelName);
        return modelFile.getAbsolutePath();
    }

    private static File getFileFromAsset(Context context, String fileName) {
        File file = null;

        try {
            InputStream is = context.getResources().getAssets().open(fileName);
            file = new File(context.getCacheDir().getAbsolutePath() + "/" + fileName);
            file.deleteOnExit();
            Log.d(TAG, "Asset file path = " + file.getAbsolutePath());

            FileOutputStream os = new FileOutputStream(file);
            int read;
            byte[] bytes = new byte[1024];
            while ((read = is.read(bytes)) != -1) {
                os.write(bytes, 0, read);
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to read the file : " + fileName);
        }

        return file;
    }

    public static void generateTestData() {
        int dimensionSize = 1;
        for (int i = 0; i < MOBILENET_INPUT_TENSOR_DIMENSIONS.length; i++) {
            dimensionSize *= MOBILENET_INPUT_TENSOR_DIMENSIONS[i];
        }
        byte[] byteArray = new byte[dimensionSize];
        for (int i = 0; i < dimensionSize; i++) {
            byteArray[i] = (byte) (new Random().nextInt(256));
        }

        dimensionSize = 1;
        for (int i = 0; i < POSENET_INPUT_TENSOR_DIMENSIONS.length; i++) {
            dimensionSize *= POSENET_INPUT_TENSOR_DIMENSIONS[i];
        }
        float[] floatArray = new float[dimensionSize];
        for (int i = 0; i < dimensionSize; i++) {
            floatArray[i] = new Random().nextFloat();
        }

        TEST_DATA_UINT8 = ByteBuffer.wrap(byteArray);
        TEST_DATA_FLOAT32 = FloatBuffer.wrap(floatArray);
    }

    public static void getOrangeRGBImage(Context context) {
        ImageDecoder.Source imageSource = ImageDecoder.createSource(context.getResources().getAssets(), ORANGE_IMAGE_FILE_NAME);
        try {
            Bitmap bitmap = ImageDecoder.decodeBitmap(imageSource).copy(Bitmap.Config.ARGB_8888, true);
            int width = bitmap.getWidth();
            int height = bitmap.getHeight();
            int totalPixels = width * height;
            @ColorInt int[] pixels = new int[totalPixels];
            bitmap.getPixels(pixels, 0, width, 0, 0, width, height);
            int componentsPerPixel = 3;
            byte[] rgbBytes = new byte[totalPixels * componentsPerPixel];
            for (int i = 0; i < totalPixels; i++) {
                @ColorInt int pixel = pixels[i];
                int red = Color.red(pixel);
                int green = Color.green(pixel);
                int blue = Color.blue(pixel);
                rgbBytes[i * componentsPerPixel + 0] = (byte) red;
                rgbBytes[i * componentsPerPixel + 1] = (byte) green;
                rgbBytes[i * componentsPerPixel + 2] = (byte) blue;
            }
            TEST_ORANGE_IMAGE = ByteBuffer.wrap(rgbBytes);
        } catch (IOException e) {
            Log.e(TAG, "Fail to get an orange image.");
        }
    }

    public static void organizeLabelMap(Context context) {
        File labelFile = getFileFromAsset(context, LABEL_FILE_NAME);
        try {
            FileReader fileReader = new FileReader(labelFile);
            BufferedReader bufferedReader = new BufferedReader(fileReader);
            int count = 1;
            String line;
            while ((line = bufferedReader.readLine()) != null) {
                labelMap.put(count++, line);
            }
        } catch (Exception e) {
            Log.e(TAG, "Fail to organize a label map.");
        }
        Log.i(TAG, "Finished");
    }
}
