#include "CoreWrapper.h"

#include <odr/OpenDocumentReader.h>
#include <odr/Config.h>
#include <odr/Meta.h>

JNIEXPORT jobject JNICALL
Java_at_tomtasche_reader_background_CoreWrapper_parseNative(JNIEnv *env, jobject instance, jobject options)
{
    jboolean isCopy;

    jclass optionsClass = env->GetObjectClass(options);
    jfieldID pointerField = env->GetFieldID(optionsClass, "nativePointer", "J");

    odr::OpenDocumentReader *translator;

    jlong pointer = env->GetLongField(options, pointerField);
    if (pointer == 0) {
        translator = new odr::OpenDocumentReader();
    } else {
        translator = (odr::OpenDocumentReader *) pointer;
    }

    jclass resultClass = env->FindClass("at/tomtasche/reader/background/CoreWrapper$CoreResult");
    jmethodID resultConstructor = env->GetMethodID(resultClass, "<init>", "()V");
    jobject result = env->NewObject(resultClass, resultConstructor);

    jfieldID pointerResultField = env->GetFieldID(resultClass, "nativePointer", "J");
    env->SetLongField(result, pointerResultField, reinterpret_cast<jlong>(translator));

    jfieldID errorField = env->GetFieldID(resultClass, "errorCode", "I");

    jfieldID inputPathField = env->GetFieldID(optionsClass, "inputPath", "Ljava/lang/String;");
    jstring inputPath = (jstring) env->GetObjectField(options, inputPathField);

    const char *inputPathC = env->GetStringUTFChars(inputPath, &isCopy);
    std::string inputPathCpp = std::string(inputPathC, env->GetStringUTFLength(inputPath));
    env->ReleaseStringUTFChars(inputPath, inputPathC);

    try {
        bool opened = translator->open(inputPathCpp);
        if (!opened) {
            env->SetIntField(result, errorField, -1);
            return result;
        }

        auto meta = translator->getMeta();

        jfieldID passwordField = env->GetFieldID(optionsClass, "password", "Ljava/lang/String;");
        jstring password = (jstring) env->GetObjectField(options, passwordField);

        bool decrypted = !meta.encrypted;
        if (password != NULL) {
            const char *passwordC = env->GetStringUTFChars(password, &isCopy);
            std::string passwordCpp = std::string(passwordC, env->GetStringUTFLength(password));
            env->ReleaseStringUTFChars(password, passwordC);

            decrypted = translator->decrypt(passwordCpp);

            meta = translator->getMeta();
        }

        if (!decrypted) {
            env->SetIntField(result, errorField, -2);
            return result;
        }

        std::string extensionCpp = meta.typeAsString();
        const char *extensionC = extensionCpp.c_str();
        jstring extension = env->NewStringUTF(extensionC);

        jfieldID extensionField = env->GetFieldID(resultClass, "extension", "Ljava/lang/String;");
        env->SetObjectField(result, extensionField, extension);

        jfieldID editableField = env->GetFieldID(optionsClass, "editable", "Z");
        jboolean editable = env->GetBooleanField(options, editableField);

        odr::Config config = {};
        config.editable = editable;
        config.entryCount = 1;
        config.tableLimitRows = 10000;

        jfieldID outputPathField = env->GetFieldID(optionsClass, "outputPath", "Ljava/lang/String;");
        jstring outputPath = (jstring) env->GetObjectField(options, outputPathField);

        const char *outputPathC = env->GetStringUTFChars(outputPath, &isCopy);
        std::string outputPathCpp = std::string(outputPathC, env->GetStringUTFLength(outputPath));
        env->ReleaseStringUTFChars(outputPath, outputPathC);

        jclass listClass = env->FindClass("java/util/List");
        jmethodID addMethod = env->GetMethodID(listClass, "add", "(Ljava/lang/Object;)Z");

        jfieldID pageNamesField = env->GetFieldID(resultClass, "pageNames", "Ljava/util/List;");
        jobject pageNames = (jobject) env->GetObjectField(result, pageNamesField);

        jfieldID ooxmlField = env->GetFieldID(optionsClass, "ooxml", "Z");
        jboolean ooxml = env->GetBooleanField(options, ooxmlField);
        if (!ooxml) {
            if (meta.type == odr::FileType::OPENDOCUMENT_TEXT || meta.type == odr::FileType::OPENDOCUMENT_GRAPHICS) {
                jstring pageName = env->NewStringUTF("Document");
                env->CallBooleanMethod(pageNames, addMethod, pageName);

                outputPathCpp = outputPathCpp + "0.html";

                bool translated = translator->translate(outputPathCpp, config);
                if (!translated) {
                    env->SetIntField(result, errorField, -4);
                    return result;
                }
            } else if (meta.type == odr::FileType::OPENDOCUMENT_SPREADSHEET || meta.type == odr::FileType::OPENDOCUMENT_PRESENTATION) {
                int i = 0;
                // TODO: this could fail for HUGE documents with hundreds of pages
                // https://stackoverflow.com/a/24292867/198996
                for (auto page = meta.entries.begin(); page != meta.entries.end(); page++) {
                    jstring pageName = env->NewStringUTF(page->name.c_str());
                    env->CallBooleanMethod(pageNames, addMethod, pageName);

                    std::string entryOutputPath = outputPathCpp + std::to_string(i) + ".html";
                    config.entryOffset = i;

                    bool translated = translator->translate(entryOutputPath, config);
                    if (!translated) {
                        env->SetIntField(result, errorField, -4);
                        return result;
                    }

                    i++;
                }
            } else {
                env->SetIntField(result, errorField, -5);
                return result;
            }
        } else {
            if (meta.type == odr::FileType::OFFICE_OPEN_XML_DOCUMENT) {
                jstring pageName = env->NewStringUTF("Text document");
                env->CallBooleanMethod(pageNames, addMethod, pageName);

                outputPathCpp = outputPathCpp + "0.html";

                bool translated = translator->translate(outputPathCpp, config);
                if (!translated) {
                    env->SetIntField(result, errorField, -4);
                    return result;
                }
            } else if (meta.type == odr::FileType::OFFICE_OPEN_XML_WORKBOOK || meta.type == odr::FileType::OFFICE_OPEN_XML_PRESENTATION) {
                int i = 0;
                // TODO: this could fail for HUGE documents with hundreds of pages
                // https://stackoverflow.com/a/24292867/198996
                for (auto page = meta.entries.begin(); page != meta.entries.end(); page++) {
                    jstring pageName = env->NewStringUTF(page->name.c_str());
                    env->CallBooleanMethod(pageNames, addMethod, pageName);

                    std::string entryOutputPath = outputPathCpp + std::to_string(i) + ".html";
                    config.entryOffset = i;

                    bool translated = translator->translate(entryOutputPath, config);
                    if (!translated) {
                        env->SetIntField(result, errorField, -4);
                        return result;
                    }

                    i++;
                }
            } else {
                env->SetIntField(result, errorField, -5);
                return result;
            }
        }
    } catch (...) {
        env->SetIntField(result, errorField, -3);
        return result;
    }

    env->SetIntField(result, errorField, 0);
    return result;
}

JNIEXPORT jobject JNICALL
Java_at_tomtasche_reader_background_CoreWrapper_backtranslateNative(JNIEnv *env, jobject instance, jobject options, jstring htmlDiff)
{
    jboolean isCopy;

    jclass optionsClass = env->GetObjectClass(options);
    jfieldID pointerField = env->GetFieldID(optionsClass, "nativePointer", "J");

    jlong pointer = env->GetLongField(options, pointerField);
    odr::OpenDocumentReader *translator = (odr::OpenDocumentReader *) pointer;

    jclass resultClass = env->FindClass("at/tomtasche/reader/background/CoreWrapper$CoreResult");
    jmethodID resultConstructor = env->GetMethodID(resultClass, "<init>", "()V");
    jobject result = env->NewObject(resultClass, resultConstructor);

    jfieldID errorField = env->GetFieldID(resultClass, "errorCode", "I");

    try {
        jfieldID outputPathPrefixField = env->GetFieldID(optionsClass, "outputPath", "Ljava/lang/String;");
        jstring outputPathPrefix = (jstring) env->GetObjectField(options, outputPathPrefixField);

        const char *outputPathPrefixC = env->GetStringUTFChars(outputPathPrefix, &isCopy);
        std::string outputPathPrefixCpp = std::string(outputPathPrefixC, env->GetStringUTFLength(outputPathPrefix));
        env->ReleaseStringUTFChars(outputPathPrefix, outputPathPrefixC);

        const char *htmlDiffC = env->GetStringUTFChars(htmlDiff, &isCopy);
        std::string htmlDiffCpp = std::string(htmlDiffC, env->GetStringUTFLength(htmlDiff));
        env->ReleaseStringUTFChars(htmlDiff, htmlDiffC);

        const auto meta = translator->getMeta();
        std::string extension;
        if (meta.type == odr::FileType::OPENDOCUMENT_TEXT) {
            extension = "odt";
        } else if (meta.type == odr::FileType::OPENDOCUMENT_SPREADSHEET) {
            extension = "ods";
        } else if (meta.type == odr::FileType::OPENDOCUMENT_PRESENTATION) {
            extension = "odp";
        } else if (meta.type == odr::FileType::OPENDOCUMENT_GRAPHICS) {
            extension = "odg";
        } else if (meta.type == odr::FileType::OFFICE_OPEN_XML_DOCUMENT) {
            extension = "docx";
        } else if (meta.type == odr::FileType::OFFICE_OPEN_XML_WORKBOOK) {
            extension = "xlsx";
        } else if (meta.type == odr::FileType::OFFICE_OPEN_XML_PRESENTATION) {
            extension = "pptx";
        } else {
            extension = "unknown";
        }

        std::string outputPathCpp = outputPathPrefixCpp + "." + extension;
        const char *outputPathC = outputPathCpp.c_str();
        jstring outputPath = env->NewStringUTF(outputPathC);

        jfieldID outputPathField = env->GetFieldID(resultClass, "outputPath", "Ljava/lang/String;");
        env->SetObjectField(result, outputPathField, outputPath);

        bool success = translator->edit(htmlDiffCpp);
        if (!success) {
            env->SetIntField(result, errorField, -4);
            return result;
        }

        success = translator->save(outputPathCpp);
        if (!success) {
            env->SetIntField(result, errorField, -5);
            return result;
        }
    } catch (...) {
        env->SetIntField(result, errorField, -3);
        return result;
    }

    env->SetIntField(result, errorField, 0);
    return result;
}

JNIEXPORT void JNICALL
Java_at_tomtasche_reader_background_CoreWrapper_closeNative(JNIEnv *env, jobject instance, jobject options)
{
    jclass optionsClass = env->GetObjectClass(options);
    jfieldID pointerField = env->GetFieldID(optionsClass, "nativePointer", "J");

    jlong pointer = env->GetLongField(options, pointerField);
    odr::OpenDocumentReader *translator = (odr::OpenDocumentReader *) pointer;

    delete translator;
}