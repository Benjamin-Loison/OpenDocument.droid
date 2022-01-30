package at.tomtasche.reader.test;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.action.ViewActions.click;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.isEnabled;
import static androidx.test.espresso.matcher.ViewMatchers.withContentDescription;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import static androidx.test.espresso.matcher.ViewMatchers.withText;
import static org.hamcrest.Matchers.allOf;

import android.Manifest;
import android.content.Context;
import android.content.res.AssetManager;
import android.os.Environment;
import android.view.View;

import androidx.test.espresso.FailureHandler;
import androidx.test.espresso.ViewInteraction;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.LargeTest;
import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.rule.ActivityTestRule;
import androidx.test.rule.GrantPermissionRule;

import org.hamcrest.Matcher;
import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import at.tomtasche.reader.R;
import at.tomtasche.reader.ui.activity.MainActivity;

@LargeTest
@RunWith(AndroidJUnit4.class)
public class OdtActivityTest {

    private File m_testFile;

    private boolean loadingDone;

    @Rule
    public ActivityTestRule<MainActivity> mActivityTestRule = new ActivityTestRule<>(MainActivity.class);

    @Rule
    public GrantPermissionRule mGrantPermissionRule =
            GrantPermissionRule.grant(
                    Manifest.permission.WRITE_EXTERNAL_STORAGE);

    private static void copy(InputStream src, File dst) throws IOException {
        try (OutputStream out = new FileOutputStream(dst)) {
            byte[] buf = new byte[1024];
            int len;
            while ((len = src.read(buf)) > 0) {
                out.write(buf, 0, len);
            }
        }
    }

    @Before
    public void extractTestFile() throws IOException {
        File folder = new File(Environment.getExternalStorageDirectory(), "AAA_ODRTEST");
        folder.mkdirs();

        m_testFile = new File(folder, "test.odt");

        Context testCtx = InstrumentationRegistry.getInstrumentation().getContext();
        AssetManager assetManager = testCtx.getAssets();
        try (InputStream inputStream = assetManager.open("test.odt")) {
            copy(inputStream, m_testFile);
        }
    }

    @After
    public void cleanupTestFile() {
        if (null != m_testFile) {
            m_testFile.delete();
        }
    }

    @Test
    public void mainActivityTest() {
        // TODO: fix for Android 29+

        ViewInteraction actionMenuItemView = onView(
                allOf(withId(R.id.menu_open), withContentDescription("Open document"),
                        isDisplayed()));
        actionMenuItemView.perform(click());

        ViewInteraction appCompatTextView = onView(
                allOf(withId(android.R.id.text1), withText("Choose file from device"),
                        isDisplayed()));
        appCompatTextView.perform(click());

        ViewInteraction textView = onView(
                allOf(withId(android.R.id.text1), withText("AAA_ODRTEST"),
                        isDisplayed()));
        textView.perform(click());

        ViewInteraction textView2 = onView(
                allOf(withId(android.R.id.text1), withText("test.odt"),
                        isDisplayed()));
        textView2.perform(click());

        ViewInteraction appCompatButton = onView(
                allOf(withId(R.id.nnf_button_ok), withText("OK"),
                        isDisplayed()));
        appCompatButton.perform(click());

        //pressBack();

        do {
            ViewInteraction loadingDialog = onView(
                    allOf(withId(android.R.id.message), withText("This could take a few minutes, depending on the structure of your document and the processing power of your device."),
                            isDisplayed()));
            loadingDialog.withFailureHandler(new FailureHandler() {
                @Override
                public void handle(Throwable error, Matcher<View> viewMatcher) {
                    // awful code right here. official method using "IdlingResource" seems weird too though

                    loadingDone = true;
                }
            });
            loadingDialog.check(matches(withText("This could take a few minutes, depending on the structure of your document and the processing power of your device.")));
        } while (!loadingDone);

        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        ViewInteraction actionMenuItemView2 = onView(
                allOf(withId(R.id.menu_edit), withContentDescription("Edit document"),
                        isEnabled()));
        actionMenuItemView2.withFailureHandler(new FailureHandler() {
            @Override
            public void handle(Throwable error, Matcher<View> viewMatcher) {
                // fails on small screens, try again with overflow menu

                ViewInteraction overflowMenuButton = onView(
                        allOf(withContentDescription("More options"),
                                isDisplayed()));
                overflowMenuButton.perform(click());

                ViewInteraction actionMenuItemView2 = onView(
                        allOf(withId(R.id.menu_edit), withContentDescription("Edit document"),
                                isDisplayed()));
                actionMenuItemView2.perform(click());
            }
        });
    }
}
